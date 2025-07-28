#include "test_image.hpp"

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"
#include "kernel.hpp"
#include "logger/serial_appender.hpp"
#include "syscall.hpp"

#include <span>

extern "C" const km::testing::TestFactory __test_cases_start[];
extern "C" const km::testing::TestFactory __test_cases_end[];

constinit km::CpuLocal<x64::TaskStateSegment> km::tlsTaskState;

static constinit km::SystemGdt gBootGdt{};
constinit static km::SerialAppender gSerialAppender;

km::PageTables& km::GetProcessPageTables() {
    KM_PANIC("GetProcessPageTables is not implemented in the kernel test environment.");
}

sys::System *km::GetSysSystem() {
    KM_PANIC("GetSysSystem is not implemented in the kernel test environment.");
}

km::SystemMemory *km::GetSystemMemory() {
    KM_PANIC("GetSystemMemory is not implemented in the kernel test environment.");
}

void km::setupInitialGdt() {
    KM_PANIC("SetupInitialGdt is not implemented in the kernel test environment.");
}

static std::span<const km::testing::TestFactory> GetTestCases() {
    return std::span(__test_cases_start, __test_cases_end);
}

void km::testing::Test::Run() {
    SetUp();
    TestBody();
    TearDown();
}

static void NormalizeProcessorState() {
    x64::Cr0 cr0 = x64::Cr0::of(x64::Cr0::PG | x64::Cr0::WP | x64::Cr0::NE | x64::Cr0::ET | x64::Cr0::PE);
    x64::Cr0::store(cr0);

    x64::Cr4 cr4 = x64::Cr4::of(x64::Cr4::PAE);
    x64::Cr4::store(cr4);

    // Enable NXE bit
    IA32_EFER |= (1 << 11);

    IA32_GS_BASE = 0;
}

static km::SerialPortStatus InitSerialPort(km::ComPortInfo info) {
    if (km::OpenSerialResult com = OpenSerial(info)) {
        return com.status;
    } else {
        km::SerialAppender::create(com.port, &gSerialAppender);
        km::LogQueue::addGlobalAppender(&gSerialAppender);
        return km::SerialPortStatus::eOk;
    }
}

void km::testing::InitKernelTest(boot::LaunchInfo) {
    NormalizeProcessorState();
    SetDebugLogLock(DebugLogLockType::eNone);

    ComPortInfo com1Info = {
        .port = km::com::kComPort1,
        .divisor = km::com::kBaud9600,
    };

    InitSerialPort(com1Info);

    gBootGdt = getBootGdt();
    setupInitialGdt();

    LogQueue::initGlobalLogQueue(128);

    TestLog.infof("Kernel test environment initialized.");
}

int km::testing::RunAllTests() {
    for (const km::testing::TestFactory factory : GetTestCases()) {
        std::unique_ptr<km::testing::Test> test(factory());
        test->Run();
    }

    return 0;
}
