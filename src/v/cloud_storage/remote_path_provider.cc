// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0
#include "cloud_storage/remote_path_provider.h"

#include "cloud_storage/partition_manifest.h"
#include "cloud_storage/partition_path_utils.h"
#include "cloud_storage/remote_label.h"
#include "cloud_storage/spillover_manifest.h"
#include "cloud_storage/topic_path_utils.h"

namespace cloud_storage {

remote_path_provider::remote_path_provider(std::optional<remote_label> label)
  : label_(label) {}

ss::sstring remote_path_provider::topic_manifest_prefix(
  const model::topic_namespace& topic) const {
    if (label_.has_value()) {
        return labeled_topic_manifest_prefix(*label_, topic);
    }
    return prefixed_topic_manifest_prefix(topic);
}

ss::sstring remote_path_provider::topic_manifest_path(
  const model::topic_namespace& topic, model::initial_revision_id rev) const {
    if (label_.has_value()) {
        return labeled_topic_manifest_path(*label_, topic, rev);
    }
    return prefixed_topic_manifest_bin_path(topic);
}

ss::sstring remote_path_provider::partition_manifest_prefix(
  const model::ntp& ntp, model::initial_revision_id rev) const {
    if (label_.has_value()) {
        return labeled_partition_manifest_prefix(*label_, ntp, rev);
    }
    return prefixed_partition_manifest_prefix(ntp, rev);
}

ss::sstring remote_path_provider::partition_manifest_path(
  const partition_manifest& manifest) const {
    return fmt::format(
      "{}/{}",
      partition_manifest_prefix(manifest.get_ntp(), manifest.get_revision_id()),
      manifest.get_manifest_filename());
}

ss::sstring remote_path_provider::partition_manifest_path(
  const model::ntp& ntp, model::initial_revision_id rev) const {
    return fmt::format(
      "{}/{}",
      partition_manifest_prefix(ntp, rev),
      partition_manifest::filename());
}

std::optional<ss::sstring> remote_path_provider::partition_manifest_path_json(
  const model::ntp& ntp, model::initial_revision_id rev) const {
    if (label_.has_value()) {
        return std::nullopt;
    }
    return prefixed_partition_manifest_json_path(ntp, rev);
}

ss::sstring remote_path_provider::spillover_manifest_path(
  const partition_manifest& stm_manifest,
  const spillover_manifest_path_components& c) const {
    return fmt::format(
      "{}/{}",
      partition_manifest_prefix(
        stm_manifest.get_ntp(), stm_manifest.get_revision_id()),
      spillover_manifest::filename(c));
}

} // namespace cloud_storage
