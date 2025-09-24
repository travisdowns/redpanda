load("//bazel:test.bzl", "redpanda_cc_bench", "redpanda_cc_btest")

redpanda_cc_btest(
    name = "random_test",
    timeout = "short",
    srcs = [
        "random_test.cc",
    ],
    deps = [
        "//src/v/base",
        "//src/v/random:fast_prng",
        "//src/v/random:generators",
        "//src/v/test_utils:seastar_boost",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@boost//:test",
    ],
)

redpanda_cc_btest(
    name = "random_seeding_test",
    timeout = "short",
    srcs = [
        "random_seeding_test.cc",
    ],
    deps = [
        "//src/v/base",
        "//src/v/random:fast_prng",
        "//src/v/random:generators",
        "//src/v/test_utils:seastar_boost",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@boost//:test",
    ],
)

redpanda_cc_bench(
    name = "random_rpbench",
    srcs = ["random_bench.cc"],
    deps = [
        "//src/v/random:generators",
        "@seastar",
        "@seastar//:benchmark",
    ],
)
