#include <ktest/ktest.h>

#include "private.hpp"

#include "devices/qemu/debugexit.hpp"
#include "logger/categories.hpp"
#include "logger/logger.hpp"

void LaunchKernel(boot::LaunchInfo launch) {
    km::testing::detail::initKernelTest(launch);
    int result = km::testing::detail::runAllTests();

    km::QemuExitDevice exitDevice{};

    if (result != 0) {
        TestLog.fatalf("Kernel tests failed with code: ", result);
    } else {
        TestLog.infof("All kernel tests passed successfully.");
    }

    exitDevice.exit(result);
}
