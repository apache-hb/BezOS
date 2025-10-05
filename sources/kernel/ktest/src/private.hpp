#pragma once

#include "boot.hpp"

namespace km::testing::detail {
    void initKernelTest(boot::LaunchInfo launch);
    int runAllTests();
}
