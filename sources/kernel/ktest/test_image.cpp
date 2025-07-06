#include "test_image.hpp"

#include "kernel.hpp"

#include <span>

extern "C" const km::testing::TestFactory __test_cases_start[];
extern "C" const km::testing::TestFactory __test_cases_end[];

constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

km::PageTables& km::GetProcessPageTables() {
    KM_PANIC("GetProcessPageTables is not implemented in the kernel test environment.");
}

sys::System *km::GetSysSystem() {
    KM_PANIC("GetSysSystem is not implemented in the kernel test environment.");
}

km::SystemMemory *km::GetSystemMemory() {
    KM_PANIC("GetSystemMemory is not implemented in the kernel test environment.");
}

static std::span<const km::testing::TestFactory> GetTestCases() {
    return std::span(__test_cases_start, __test_cases_end);
}

void km::testing::Test::Run() {
    SetUp();
    TestBody();
    TearDown();
}

void km::testing::InitKernelTest(boot::LaunchInfo) {

}

int km::testing::RunAllTests() {
    for (const km::testing::TestFactory factory : GetTestCases()) {
        std::unique_ptr<km::testing::Test> test(factory());
        test->Run();
    }

    return 0;
}
