load("//rules/opentitan:defs.bzl", "opentitan_test")

opentitan_test(
    name = "tpm_minimal_test",
    srcs = ["tpm_minimal_test.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
    },
    deps = [
        ":tpm_driver",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:ottf_main",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "tpm_driver",
    srcs = ["tpm_driver.c"],
    hdrs = ["tpm_hal.h"],
    deps = [
        "//sw/device/lib/runtime:log",
    ],
)
