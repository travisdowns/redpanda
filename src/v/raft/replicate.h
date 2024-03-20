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

#pragma once

#include "base/outcome.h"
#include "model/fundamental.h"
#include "raft/errc.h"
#include "utils/oc_latency_fwd.h"

#include <chrono>
#include <optional>

namespace raft {

enum class consistency_level { quorum_ack, leader_ack, no_ack };

struct replicate_options {
    explicit replicate_options(consistency_level l, shared_tracker tracker = {})
      : consistency(l)
      , timeout(std::nullopt)
      , tracker{std::move(tracker)} {}

    replicate_options(
      consistency_level l,
      std::chrono::milliseconds timeout,
      shared_tracker tracker = {})
      : consistency(l)
      , timeout(timeout)
      , tracker{std::move(tracker)} {}

    // Callers may choose to force flush on an individual replicate request
    // basis. This is useful if certain callers intend to override any
    // default behavior at global/topic scope.
    // For example: when write caching is enabled on the topic and a caller
    // can still force a flush with this override. This override takes
    // precendence over any other setting.
    void set_force_flush() { _force_flush = true; }
    bool force_flush() const { return _force_flush; }

    consistency_level consistency;
    std::optional<std::chrono::milliseconds> timeout;
    shared_tracker tracker;
    bool _force_flush = false;
};

struct replicate_result {
    /// used by the kafka API to produce a kafka reply to produce request.
    /// see produce_request.cc
    model::offset last_offset;
};

struct replicate_stages {
    replicate_stages(ss::future<>, ss::future<result<replicate_result>>);
    explicit replicate_stages(raft::errc);
    // after this future is ready, request in enqueued in raft and it will not
    // be reorderd
    ss::future<> request_enqueued;
    // after this future is ready, request was successfully replicated with
    // requested consistency level
    ss::future<result<replicate_result>> replicate_finished;
};

} // namespace raft
