load("//bazel:build.bzl", "redpanda_cc_library")

redpanda_cc_library(
    name = "fast_prng",
    hdrs = [
        "fast_prng.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/random",
    ],
)

redpanda_cc_library(
    name = "generators",
    srcs = [
        "generators.cc",
    ],
    hdrs = [
        "generators.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "secure_random",
    srcs = [
        "secure_generators.cc",
    ],
    hdrs = [
        "secure_generators.h",
    ],
    implementation_deps = [
        ":generators",
        "//src/v/crypto",
    ],
    include_prefix = "random",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "time_jitter",
    hdrs = [
        "simple_time_jitter.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":fast_prng",
    ],
)
