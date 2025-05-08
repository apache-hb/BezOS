#include "system_test.hpp"

#include "fs/vfs.hpp"

struct TestData {
    km::SystemMemory memory;
    vfs::VfsRoot vfs;
    sys::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(2).bytes()))
    {
        OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
        if (status != OsStatusSuccess) {
            throw std::runtime_error(std::format("Failed to create system {}", status));
        }
        for (size_t i = 0; i < 4; i++) {
            system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 64);
        }
    }
};

class SystemThreadTest : public SystemBaseTest {
public:
    void SetUp() override {
        SystemBaseTest::SetUp();
        data = std::make_unique<TestData>(body);
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

    void TearDown() override {
        OsStatus status = sys::SysDestroyRootProcess(system(), hRootProcess.get());
        ASSERT_EQ(status, OsStatusSuccess);
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys::ProcessHandle> hRootProcess = nullptr;

    OsProcessHandle hProcess = OS_HANDLE_INVALID;

    sys::System *system() { return &data->system; }
    sys::InvokeContext invoke() {
        return sys::InvokeContext { system(), GetProcess(hRootProcess->getProcess(), hProcess) };
    }

    sys::InvokeContext invokeRoot() {
        return sys::InvokeContext { system(), hRootProcess->getProcess() };
    }
};

TEST_F(SystemThreadTest, StatThread) {
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

    OsThreadInfo info{};
    status = sys::SysThreadStat(&i, hThread, &info);
    ASSERT_EQ(status, OsStatusSuccess) << "Thread stat failed";
    ASSERT_EQ(info.State, eOsThreadQueued) << "Thread state is not running";
    ASSERT_STREQ(info.Name, "TEST") << "Thread name is not TEST";

    OsProcessInfo myInfo{}, otherInfo{};
    status = sys::SysProcessStat(&ir, hProcess, &myInfo);
    ASSERT_EQ(status, OsStatusSuccess) << "Process stat failed";
    status = sys::SysProcessStat(&i, info.Process, &otherInfo);
    ASSERT_EQ(status, OsStatusSuccess) << "Process stat failed";

    ASSERT_EQ(myInfo.Id, otherInfo.Id) << "Thread parent is not the same as process";
}
