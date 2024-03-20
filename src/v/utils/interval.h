#pragma once

#include "base/seastarx.h"
#include "base/vassert.h"
#include "oc_latency_fwd.h"

#include <seastar/core/sstring.hh>

namespace interval {

using default_clock = std::chrono::steady_clock;

struct event_record {
    const char* what;
    std::chrono::nanoseconds when; // offset from start time
};

struct interval {
    using clock = default_clock;
    using tp = default_clock::time_point;

    explicit interval(ss::sstring name, tp start = clock::now())
      : _name(std::move(name))
      , _start(start) {}

    ~interval() { stop(); }

    void stop();

    interval(const interval&) = delete;
    interval& operator=(const interval&) = delete;

    interval& operator=(interval&& rhs) noexcept {
        _name = std::move(rhs._name);
        _start = std::move(rhs._start);
        _alive = std::move(rhs._alive);
        rhs._alive = false;

        return *this;
    }

    interval(interval&& rhs) noexcept { *this = std::move(rhs); }

    bool alive() { return _alive; }

    seastar::sstring _name;
    default_clock::time_point _start;
    bool _alive = true;
};

inline interval start_interval(ss::sstring name) {
    return interval{std::move(name)};
}

} // namespace interval
