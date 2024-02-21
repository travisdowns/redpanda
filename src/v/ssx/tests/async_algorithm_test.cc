// Copyright 2023 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0
#include "base/seastarx.h"
#include "gmock/gmock.h"
#include "ssx/async-clear.h"
#include "ssx/async_algorithm.h"
#include "ssx/future-util.h"

#include <seastar/core/reactor.hh>
#include <seastar/core/thread.hh>
#include <seastar/coroutine/maybe_yield.hh>

#include <algorithm>
#include <cstdint>
#include <iterator>

namespace ssx {

namespace {

static ss::future<> async_push_back(auto& container, ssize_t n, auto& val) {
    constexpr ssize_t batch = 1000;

    while (n > 0) {
        std::fill_n(std::back_inserter(container), std::min(n, batch), val);
        n -= batch;
        co_await ss::coroutine::maybe_yield();
    }
}

const auto add_one = [](int& x) { x++; };
[[maybe_unused]] const auto add_one_slow = [](int& x) {
    static volatile int sink = 0;
    for (int i = 0; i < 100; i++) {
        sink = sink + 1;
    }
    x++;
};

static thread_local int yields;

template<ssize_t I>
struct test_traits : async_algo_traits {
    constexpr static ssize_t Interval = I;
    static void yield_called() { yields++; }
};

static void check_same_result(const auto& container, auto fn) {
    auto std_input = container, async_input0 = container,
         async_input1 = container, async_input2 = container,
         async_input3 = container;

    std::for_each(std_input.begin(), std_input.end(), fn);

    // vanilla
    ssx::async_for_each(async_input0.begin(), async_input0.end(), fn).get();

    // interval 1
    ssx::async_for_each<test_traits<1>>(
      async_input1.begin(), async_input1.end(), fn)
      .get();

    // vanilla with counter
    async_counter c;
    ssx::async_for_each_counter(c, async_input2.begin(), async_input2.end(), fn)
      .get();

    // interval 1 with counter
    ssx::async_for_each_counter<test_traits<1>>(
      c, async_input3.begin(), async_input3.end(), fn)
      .get();

    EXPECT_EQ(std_input, async_input0);
    EXPECT_EQ(std_input, async_input1);
    EXPECT_EQ(std_input, async_input2);
    EXPECT_EQ(std_input, async_input3);
};

struct task_counter {
    static int64_t tasks() {
        return (int64_t)ss::engine().get_sched_stats().tasks_processed;
    }

    int64_t task_start = tasks();
    ssize_t yield_start = yields;

    int64_t task_delta() { return tasks() - task_start; }
    ssize_t yield_delta() { return yields - yield_start; }
};

} // namespace

TEST(AsyncAlgo, async_for_each_same_result) {
    // basic checks
    check_same_result(std::vector<int>{}, add_one);
    check_same_result(std::vector<int>{1, 2, 3, 4}, add_one);
}

TEST(AsyncAlgo, yield_count) {
    // helper to check that async_for_each results in the same final state
    // as std::for_each

    std::vector<int> v{1, 2, 3, 4, 5};

    task_counter c;
    ssx::async_for_each<test_traits<1>>(v.begin(), v.end(), add_one).get();
    EXPECT_EQ(5, c.yield_delta());

    c = {};
    ssx::async_for_each<test_traits<2>>(v.begin(), v.end(), add_one).get();
    // floor(5/2), as we don't yield on partial intervals
    EXPECT_EQ(2, c.yield_delta());
}

TEST(AsyncAlgo, yield_count_counter) {
    async_counter a_counter;

    std::vector<int> v{1, 2};

    task_counter t_counter;
    ssx::async_for_each_counter<test_traits<3>>(
      a_counter, v.begin(), v.end(), add_one)
      .get();
    EXPECT_EQ(0, t_counter.yield_delta());
    EXPECT_EQ(3, a_counter.count);

    // now we should get a yield since we carry over the 2 ops
    // from above
    t_counter = {};
    ssx::async_for_each_counter<test_traits<3>>(
      a_counter, v.begin(), v.end(), add_one)
      .get();
    EXPECT_EQ(1, t_counter.yield_delta());

    v = {1, 2, 3};
    t_counter = {};
    a_counter = {};
    ssx::async_for_each_counter<test_traits<2>>(
      a_counter, v.begin(), v.end(), add_one)
      .get();
    // 3 elems - 2 interval, overflow by 1 + FIXED_COST = 2
    EXPECT_EQ(2, a_counter.count);
    EXPECT_EQ(1, t_counter.yield_delta());

    t_counter = {};
    ssx::async_for_each_counter<test_traits<2>>(
      a_counter, v.begin(), v.end(), add_one)
      .get();
    EXPECT_EQ(2, a_counter.count);
    EXPECT_EQ(2, t_counter.yield_delta());
}

TEST(AsyncAlgo, yield_count_counter_empty) {
    async_counter a_counter;
    task_counter t_counter;

    std::vector<int> empty;

    auto call = [&] {
        ssx::async_for_each_counter<test_traits<2>>(
          a_counter, empty.begin(), empty.end(), add_one)
          .get();
    };

    call();
    EXPECT_EQ(1, a_counter.count);
    EXPECT_EQ(0, t_counter.yield_delta());

    call();
    EXPECT_EQ(2, a_counter.count);
    EXPECT_EQ(0, t_counter.yield_delta());

    call();
    EXPECT_EQ(1, a_counter.count);
    EXPECT_EQ(1, t_counter.yield_delta());

    a_counter = {};
    t_counter = {};
    constexpr auto iters = 101;
    for (int i = 0; i < iters; i++) {
        call();
    }

    // here we check that even though the vector is empty, we do yield 50 times
    // (100 iterations / work interval of 2), since always assume 1 unit of work
    // has been performed
    EXPECT_EQ(iters / 2, t_counter.yield_delta());
}

TEST(AsyncAlgo, async_for_each_large_container) {
    constexpr size_t size = 1'000'000;
    constexpr int answer = 42;

    std::deque<int> v;
    async_push_back(v, size, answer).get();

    task_counter tasks;

    async_for_each(v.begin(), v.end(), add_one_slow).get();

    EXPECT_GT(tasks.task_delta(), 1);
}

} // namespace ssx
