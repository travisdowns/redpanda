#include "utils/interval.h"

#include "base/seastarx.h"
#include "base/vlog.h"

#include <seastar/core/metrics.hh>

#include <absl/container/node_hash_map.h>

#include <chrono>

namespace interval {

static ss::logger interval_log("interval");

static constexpr std::chrono::milliseconds wait_threshold{100};

struct interval_metrics {
    /**
     * @brief Add a new series with the event label set to the given value.
     *
     * @param what
     */
    log_hist_internal& hist_for_event(ss::sstring what) {
        auto [it, added] = _event_hists.try_emplace(what);

        if (added) [[unlikely]] {
            const ss::metrics::label event_label{"event"};

            _metrics.add_group(
              "latency_interval",
              {ss::metrics::make_histogram(
                "oc_lat",
                ss::metrics::description("oclat intervals"),
                {event_label(what)},
                [&hist
                 = it->second] { // relies on node_hash_map pointer stability
                    return hist.internal_histogram_logform();
                })});
        }

        return it->second;
    }

    log_hist_internal _hist;
    ss::metrics::metric_groups _metrics;
    absl::node_hash_map<ss::sstring, log_hist_internal> _event_hists;

    static thread_local interval_metrics
      _local_instance; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local interval_metrics interval_metrics::_local_instance;

void interval::stop() {
    if (!_alive) {
        return;
    }

    auto duration = clock::now() - _start;

    interval_metrics::_local_instance.hist_for_event(_name).record(
      duration / std::chrono::microseconds(1));

    if (duration >= wait_threshold) {
        vlog(
          interval_log.info,
          "INTERVAL_WAIT {} ms: {}",
          duration / std::chrono::nanoseconds(1) / 1000000.,
          _name);
    }

    _alive = false;
}

} // namespace interval
