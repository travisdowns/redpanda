load("@bazel_skylib//rules:common_settings.bzl", "int_flag")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

# Make this build faster by setting `build --@hwloc//:build_jobs=16` in user.bazelrc
# if you have the cores to spare.
int_flag(
    name = "build_jobs",
    build_setting_default = 8,
    make_variable = "BUILD_JOBS",
)

filegroup(
    name = "srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "hwloc",
    args = ["-j$HWLOC_BUILD_JOBS"],
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    configure_in_place = True,
    configure_options = [
        "--disable-libudev",

        # Disable graphics and the many kinds of display driver discovery
        "--disable-gl",
        "--disable-opencl",
        "--disable-nvml",
        "--disable-cuda",
        "--disable-rsmi",

        # Build a static library
        "--disable-shared",
        "--enable-static",

        # Use a fixed runstatedir so the autoconf-derived path doesn't embed
        # the sandbox directory into compiled objects.
        "--runstatedir=/var/run/hwloc",
    ],
    env = {
        "HWLOC_BUILD_JOBS": "$(BUILD_JOBS)",
        # Remap two prefixes so neither the sandbox execroot nor the
        # output_base path leak into __FILE__ expansions or DWARF debug info:
        #   1. $EXT_BUILD_ROOT — the sandbox's execroot (covers hwloc's own
        #      sources and generated headers under bazel-out/).
        #   2. ${EXT_BUILD_ROOT%/sandbox/*}/external — the output_base's
        #      external/ dir (covers clang-toolchain headers such as
        #      __stddef_size_t.h and the x86_64 sysroot, which are accessed
        #      via their absolute output_base path because cc_wrapper.sh calls
        #      realpath() on itself before invoking clang, resolving the sandbox
        #      symlink to the real output_base path). See CORE-16319.
        "CFLAGS": (
            "-ffile-prefix-map=$$EXT_BUILD_ROOT=. " +
            "-ffile-prefix-map=$${EXT_BUILD_ROOT%/sandbox/*}/external=external"
        ),
        "CXXFLAGS": (
            "-ffile-prefix-map=$$EXT_BUILD_ROOT=. " +
            "-ffile-prefix-map=$${EXT_BUILD_ROOT%/sandbox/*}/external=external"
        ),
    },
    lib_source = ":srcs",
    out_binaries = [
        "hwloc-calc",
        "hwloc-distrib",
    ],
    out_static_libs = ["libhwloc.a"],
    toolchains = [":build_jobs"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@libpciaccess",
    ],
)

filegroup(
    name = "hwloc_calc",
    srcs = [":hwloc"],
    output_group = "hwloc-calc",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "hwloc_distrib",
    srcs = [":hwloc"],
    output_group = "hwloc-distrib",
    visibility = ["//visibility:public"],
)
