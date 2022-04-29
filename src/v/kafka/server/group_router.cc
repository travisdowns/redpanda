/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#include "kafka/server/group_router.h"

#include "random/generators.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/when_all.hh>
#include <seastar/core/with_scheduling_group.hh>

namespace kafka {

ss::future<std::vector<deletable_group_result>>
group_router::route_delete_groups(
  ss::shard_id shard, std::vector<std::pair<model::ntp, group_id>> groups) {
    return ss::with_scheduling_group(
      _sg, [this, shard, groups = std::move(groups)]() mutable {
          return get_group_manager().invoke_on(
            shard,
            _ssg,
            [groups = std::move(groups)](group_manager& mgr) mutable {
                return mgr.delete_groups(std::move(groups));
            });
      });
}

ss::future<> group_router::parallel_route_delete_groups(
  std::vector<deletable_group_result>& results,
  sharded_groups& groups_by_shard) {
    return ss::parallel_for_each(
      groups_by_shard, [this, &results](sharded_groups::value_type& groups) {
          return route_delete_groups(groups.first, std::move(groups.second))
            .then([&results](std::vector<deletable_group_result> new_results) {
                results.insert(
                  results.end(), new_results.begin(), new_results.end());
            });
      });
}

ss::future<std::vector<deletable_group_result>>
group_router::delete_groups(std::vector<group_id> groups) {
    // partial results
    std::vector<deletable_group_result> results;

    // partition groups by owner shard
    sharded_groups groups_by_shard;
    for (auto& group : groups) {
        if (unlikely(_disabled)) {
            results.push_back(deletable_group_result{
              .group_id = std::move(group),
              .error_code = error_code::not_coordinator,
            });
            continue;
        }

        if (auto m = shard_for(group); m) {
            groups_by_shard[m->second].emplace_back(
              std::make_pair(std::move(m->first), std::move(group)));
        } else {
            results.push_back(deletable_group_result{
              .group_id = std::move(group),
              .error_code = error_code::not_coordinator,
            });
        }
    }

    return ss::do_with(
      std::move(results),
      std::move(groups_by_shard),
      [this](
        std::vector<deletable_group_result>& results,
        sharded_groups& groups_by_shard) {
          return parallel_route_delete_groups(results, groups_by_shard)
            .then([&results] { return std::move(results); });
      });
}

ss::future<described_group> group_router::describe_group(kafka::group_id g) {
    auto unique = random_generators::get_int<unsigned>();
    vlog(kgrouplog.trace, "describe_group begin for {} id:{}", g, unique);
    auto m = shard_for(g);
    if (!m) {
        vlog(kgrouplog.trace, "describe_group failing for {}", g);
        return ss::make_ready_future<described_group>(
          describe_groups_response::make_empty_described_group(
            std::move(g), error_code::not_coordinator));
    }
    return with_scheduling_group(
      _sg,
      [this, g = std::move(g), m = std::move(m), unique = unique]() mutable {
          return get_group_manager().invoke_on(
            m->second,
            _ssg,
            [g = std::move(g), ntp = std::move(m->first), unique = unique](
              group_manager& mgr) mutable {
                auto start_time = ss::lowres_clock::now();
                vlog(
                  kgrouplog.trace,
                  "describe_group inner begin for {} id:{}",
                  g,
                  unique);
                try {
                    auto res = mgr.describe_group(ntp, g);
                    vlog(
                      kgrouplog.trace,
                      "describe_group inner success for {} in {} ms, id:{}",
                      g,
                      ss::lowres_clock::now() - start_time, unique);
                    return res;
                } catch (...) {
                    vlog(
                      kgrouplog.trace,
                      "describe_group inner failure for {} in {} ms",
                      g,
                      ss::lowres_clock::now() - start_time);
                    throw;
                }
            });
      });
}

} // namespace kafka
