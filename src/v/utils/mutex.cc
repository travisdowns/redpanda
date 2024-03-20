/*
 * Copyright 2024 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#include "utils/mutex.h"

#include "base/vassert.h"
#include "base/vlog.h"

#include <seastar/core/lowres_clock.hh>

#include <chrono>

static ss::logger mutex_log("mutex");

static constexpr std::chrono::milliseconds wait_threshold{100};

using namespace std::chrono;

ss::future<mutex::units> mutex::get_units_slow(ss::future<mutex::units> units) {
    vassert(!units.available(), "available");
    auto before = ss::lowres_clock::now();
    return std::move(units).then([=](mutex::units&& units) {
        auto wait_time = ss::lowres_clock::now() - before;
        if (wait_time >= wait_threshold) {
            vlog(
              mutex_log.info,
              "MUTEX_WAIT {} ms: {}",
              wait_time / 1ns / 1000000,
              _sem.name());
        }
        return std::move(units);
    });
}
