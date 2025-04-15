#include "system_test.hpp"

#include "fs2/vfs.hpp"

struct TestData {
    km::SystemMemory memory;
    vfs2::VfsRoot vfs;
    sys2::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(2).bytes()))
        , system(&memory.pageTables(), &memory.pmmAllocator(), &vfs)
    {
        for (size_t i = 0; i < 4; i++) {
            system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 64);
        }
    }
};

class DeviceTest : public SystemBaseTest {
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

        sys2::InvokeContext invoke { system(), hRootProcess->getProcess() };
        OsProcessCreateInfo childInfo {
            .Name = "CHILD",
        };
        status = sys2::SysProcessCreate(&invoke, childInfo, &hProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hProcess, OS_HANDLE_INVALID) << "Child process was not created";
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
        return sys2::InvokeContext { system(), GetProcess(hRootProcess->getProcess(), hProcess) };
    }
};

TEST_F(DeviceTest, CreateDevice) {
    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::DeviceOpenInfo openInfo {
        .path = vfs2::BuildPath("Root"),
        .flags = eOsDeviceCreateNew,
        .interface = kOsFolderGuid,
    };

    ASSERT_EQ(sys2::SysDeviceOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    ASSERT_EQ(sys2::SysDeviceClose(&i, node), OsStatusSuccess);

    sys2::DeviceOpenInfo openInfo2 {
        .path = vfs2::BuildPath("Root", "File.txt"),
        .flags = eOsDeviceCreateNew,
        .interface = kOsFileGuid,
    };

    ASSERT_EQ(sys2::SysDeviceOpen(&i, openInfo2, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";
}
