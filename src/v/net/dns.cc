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
#include "net/dns.h"

#include "net/unresolved_address.h"
#include "rpc/logger.h"
#include "utils/mutex.h"
#include "vlog.h"

#include <seastar/core/coroutine.hh>
#include <seastar/net/dns.hh>
#include <seastar/util/defer.hh>

#include <__chrono/duration.h>

#include <chrono>

namespace net {

ss::future<ss::socket_address> resolve_dns(unresolved_address address) {
    static thread_local ss::net::dns_resolver resolver;
    static thread_local mutex m;

    // lock
    auto units = co_await m.get_units();
    auto before_ts = std::chrono::high_resolution_clock::now();

    auto defer_log = ss::defer([before_ts, address] {
        auto duration = std::chrono::high_resolution_clock::now() - before_ts;
        auto duration_us
          = std::chrono::duration_cast<std::chrono::microseconds>(duration);
        vlog(
          rpc::rpclog.warn,
          "resolve_dns {} ms for {}",
          duration_us.count() / 1000.,
          address);
    });

    // resolve
    auto i_a = co_await resolver.resolve_name(address.host(), address.family());

    co_return ss::socket_address(i_a, address.port());
};

} // namespace net
