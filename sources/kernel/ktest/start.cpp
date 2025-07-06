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
}

void LaunchKernel(boot::LaunchInfo launch) {
    km::testing::InitKernelTest(launch);
    int result = km::testing::RunAllTests();

    if (result != 0) {
        TestLog.fatalf("Kernel tests failed with code: ", result);
        KM_PANIC("Kernel tests failed.");
    } else {
        TestLog.infof("All kernel tests passed successfully.");
    }

    KmHalt();
}
