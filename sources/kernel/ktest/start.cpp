#include "devices/qemu/debugexit.hpp"
#include "logger/categories.hpp"
#include "test_image.hpp"
#include "logger/logger.hpp"

class KernelStartupTest : public km::testing::Test {
public:
    void SetUp() override {

    }

    void TearDown() override {

    }
};

KTEST_F(KernelStartupTest, BasicTest) {
    TestLog.infof("Kernel startup test is running.");
    KASSERT_EQ(1, 1);
}

void LaunchKernel(boot::LaunchInfo launch) {
    km::testing::initKernelTest(launch);
    int result = km::testing::runAllTests();

    km::QemuExitDevice exitDevice{};

    if (result != 0) {
        TestLog.fatalf("Kernel tests failed with code: ", result);
    } else {
        TestLog.infof("All kernel tests passed successfully.");
    }

    exitDevice.exit(result);
}
