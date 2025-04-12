#include "system_test.hpp"

#include "fs2/vfs.hpp"

struct TestData {
    km::SystemMemory memory;
    vfs2::VfsRoot vfs;
    sys2::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(2).bytes()))
        , system({ 128, 128 }, &memory.pageTables(), &memory.pmmAllocator(), &vfs)
    { }
};

class SystemThreadTest : public SystemBaseTest {
public:
    void SetUp() override {
        SystemBaseTest::SetUp();
        data = std::make_unique<TestData>(body);
        sys2::ProcessCreateInfo createInfo {
            .name = "MASTER",
            .supervisor = false,
        };

        OsStatus status = sys2::SysCreateRootProcess(system(), createInfo, std::out_ptr(hRootProcess));
        ASSERT_EQ(status, OsStatusSuccess);

        sys2::InvokeContext invoke { system(), hRootProcess->getProcess(), OS_HANDLE_INVALID };
        OsProcessCreateInfo childInfo {
            .Name = "CHILD",
        };
        status = sys2::SysCreateProcess(&invoke, childInfo, &hProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hProcess, OS_HANDLE_INVALID) << "Child process was not created";

        ASSERT_TRUE(GetProcess(hRootProcess->getProcess(), hProcess) != nullptr) << "Child process is not a child of the root process";
    }

    void TearDown() override {
        OsStatus status = sys2::SysDestroyRootProcess(system(), hRootProcess.get());
        ASSERT_EQ(status, OsStatusSuccess);
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys2::ProcessHandle> hRootProcess = nullptr;

    OsProcessHandle hProcess = OS_HANDLE_INVALID;

    sys2::System *system() { return &data->system; }
    sys2::InvokeContext invoke() {
        return sys2::InvokeContext { system(), GetProcess(hRootProcess->getProcess(), hProcess), OS_HANDLE_INVALID };
    }

    sys2::InvokeContext invokeRoot() {
        return sys2::InvokeContext { system(), hRootProcess->getProcess(), OS_HANDLE_INVALID };
    }
};

TEST_F(SystemThreadTest, StatThread) {
    auto i = invoke();
    auto ir = invokeRoot();
    OsThreadHandle hThread = OS_HANDLE_INVALID;
    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };

    OsStatus status = sys2::SysCreateThread(&i, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess) << "Thread was not created";
    ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread handle was not created";

    OsThreadInfo info{};
    status = sys2::SysThreadStat(&i, hThread, &info);
    ASSERT_EQ(status, OsStatusSuccess) << "Thread stat failed";
    ASSERT_EQ(info.State, eOsThreadRunning) << "Thread state is not running";
    ASSERT_STREQ(info.Name, "TEST") << "Thread name is not TEST";

    OsProcessInfo myInfo{}, otherInfo{};
    status = sys2::SysProcessStat(&ir, hProcess, &myInfo);
    ASSERT_EQ(status, OsStatusSuccess) << "Process stat failed";
    status = sys2::SysProcessStat(&i, info.Process, &otherInfo);
    ASSERT_EQ(status, OsStatusSuccess) << "Process stat failed";

    ASSERT_EQ(myInfo.Id, otherInfo.Id) << "Thread parent is not the same as process";
}
