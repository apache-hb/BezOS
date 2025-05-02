#include "system_test.hpp"
#include "test/ktest.hpp"

#include "fs2/vfs.hpp"
#include "thread.hpp"

struct TestData {
    km::SystemMemory memory;
    vfs2::VfsRoot vfs;
    sys::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(16).bytes()))
    {
        if (OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system)) {
            throw std::runtime_error(std::format("Failed to create system {}", status));
        }
    }
};

class Ia32FsBaseMsr : public kmtest::IMsrDevice {
    void wrmsr(uint32_t msr, uint64_t value) override {
        if (msr != decltype(IA32_FS_BASE)::kRegister) {
            dprintf(2, "unknown msr %u\n", msr);
            exit(1);
        }

        mFsBase = value;
    }

    uint64_t rdmsr(uint32_t msr) override {
        if (msr != decltype(IA32_FS_BASE)::kRegister) {
            dprintf(2, "unknown msr %u\n", msr);
            exit(1);
        }

        return mFsBase;
    }

public:
    uintptr_t mFsBase = 0;
};

class ScheduleTest : public SystemBaseTest {
public:
    static void SetUpTestSuite() {
        kmtest::Machine::setup();
    }

    static void TearDownTestSuite() {
        kmtest::Machine::finalize();
    }

    void SetUp() override {
        kmtest::Machine *machine = new kmtest::Machine();
        machine->msr(decltype(IA32_FS_BASE)::kRegister, &mFsBase);
        machine->reset(machine);

        SystemBaseTest::SetUp();
    }

    void TearDown() override {
        OsStatus status = sys::SysDestroyRootProcess(system(), hRootProcess.get());
        ASSERT_EQ(status, OsStatusSuccess);
    }

    void init(size_t cpus, size_t tasks) {
        data = std::make_unique<TestData>(body);
        for (size_t i = 0; i < cpus; i++) {
            system()->scheduler()->initCpuSchedule(km::CpuCoreId(i), tasks);
        }

        sys::ProcessCreateInfo createInfo {
            .name = "MASTER",
            .supervisor = false,
        };

        OsStatus status = sys::SysCreateRootProcess(system(), createInfo, std::out_ptr(hRootProcess));
        ASSERT_EQ(status, OsStatusSuccess);

        sys::InvokeContext invoke { system(), hRootProcess->getProcess() };
        OsProcessCreateInfo childInfo {
            .Name = "CHILD",
        };
        status = sys::SysProcessCreate(&invoke, childInfo, &hProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hProcess, OS_HANDLE_INVALID) << "Child process was not created";

        ASSERT_TRUE(GetProcess(hRootProcess->getProcess(), hProcess) != nullptr) << "Child process is not a child of the root process";
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys::ProcessHandle> hRootProcess = nullptr;

    OsProcessHandle hProcess = OS_HANDLE_INVALID;

    Ia32FsBaseMsr mFsBase;

    sys::System *system() { return &data->system; }
    sys::InvokeContext invoke() {
        return sys::InvokeContext { system(), GetProcess(hRootProcess->getProcess(), hProcess) };
    }

    sys::InvokeContext invokeRoot() {
        return sys::InvokeContext { system(), hRootProcess->getProcess() };
    }
};

TEST_F(ScheduleTest, StartThread) {
    init(1, 64);

    auto i = invoke();
    auto ir = invokeRoot();
    OsThreadHandle hThread = OS_HANDLE_INVALID;
    sys::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };

    OsStatus status = sys::SysThreadCreate(&i, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess) << "Thread was not created";
    ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread handle was not created";

    auto sched = system()->scheduler();
    auto local = sched->getCpuSchedule(km::CpuCoreId(0));
    ASSERT_TRUE(local != nullptr) << "Local schedule was not created";
    km::IsrContext current{};
    km::IsrContext next{};
    ASSERT_NE(local->tasks(), 0) << "No tasks in the local schedule";
    void *syscallStack = nullptr;
    ASSERT_TRUE(local->scheduleNextContext(&current, &next, &syscallStack)) << "Thread was not scheduled";
    ASSERT_TRUE(local->currentThread() != nullptr) << "Current thread was not set";
}

TEST_F(ScheduleTest, StartManyThreads) {
    init(1, 128);

    auto ivc = invoke();
    auto ir = invokeRoot();

    for (size_t i = 0; i < 100; i++) {
        OsThreadHandle hThread = OS_HANDLE_INVALID;
        sys::ThreadCreateInfo threadCreateInfo {
            .name = "TEST",
            .cpuState = {},
            .tlsAddress = 0,
            .state = eOsThreadRunning,
        };

        OsStatus status = sys::SysThreadCreate(&ivc, threadCreateInfo, &hThread);

        ASSERT_EQ(status, OsStatusSuccess) << "Thread was not created: " << i;
        ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread handle was not created";
    }

    auto sched = system()->scheduler();
    auto local = sched->getCpuSchedule(km::CpuCoreId(0));
    ASSERT_TRUE(local != nullptr) << "Local schedule was not created";

    size_t tasks = local->tasks();
    ASSERT_EQ(tasks, 100) << "Tasks were not created";

    km::IsrContext current{};
    km::IsrContext next{};
    void *syscallStack = nullptr;

    ASSERT_TRUE(local->scheduleNextContext(&current, &next, &syscallStack)) << "Thread was not scheduled";
    ASSERT_TRUE(local->currentThread() != nullptr) << "Current thread was not set";

    for (size_t i = 0; i < 1000; i++) {
        EXPECT_EQ(local->tasks(), tasks - 1) << "No tasks were forgotten " << i;
        ASSERT_TRUE(local->scheduleNextContext(&current, &next, &syscallStack)) << "Thread was not scheduled";
        ASSERT_TRUE(local->currentThread() != nullptr) << "Current thread was not set";
    }
}

TEST_F(ScheduleTest, MultiThreadSchedule) {
    static constexpr size_t kThreadCount = 8;

    init(kThreadCount, 16);

    auto ivc = invoke();
    auto ir = invokeRoot();

    for (size_t i = 0; i < 100; i++) {
        OsThreadHandle hThread = OS_HANDLE_INVALID;
        sys::ThreadCreateInfo threadCreateInfo {
            .name = "TEST",
            .cpuState = {
                .rip = 0x1000 * (i + 1),
            },
            .tlsAddress = 0,
            .state = eOsThreadRunning,
        };

        OsStatus status = sys::SysThreadCreate(&ivc, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess) << "Thread was not created";
        ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread handle was not created";
    }

    auto sched = system()->scheduler();
    auto local = sched->getCpuSchedule(km::CpuCoreId(0));
    ASSERT_TRUE(local != nullptr) << "Local schedule was not created";

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([this, i] {
            auto sched = system()->scheduler();
            auto local = sched->getCpuSchedule(km::CpuCoreId(i));
            ASSERT_TRUE(local != nullptr) << "Local schedule was not created";
            km::IsrContext current{ .rip = UINTPTR_MAX };
            for (size_t j = 0; j < 1000; j++) {
                km::IsrContext next{};
                void *syscallStack = nullptr;
                ASSERT_NE(local->tasks(), 0) << "No tasks in the local schedule";
                ASSERT_TRUE(local->scheduleNextContext(&current, &next, &syscallStack)) << "Thread was not scheduled";
                ASSERT_TRUE(local->currentThread() != nullptr) << "Current thread was not set";

                ASSERT_NE(current.rip, next.rip) << "Thread was not scheduled";
                current = next;
            }
        });
    }
    threads.clear();
}
