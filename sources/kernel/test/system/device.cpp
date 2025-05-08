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

class DeviceTest : public SystemBaseTest {
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
};

TEST_F(DeviceTest, CreateDevice) {
    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::DeviceOpenInfo openInfo {
        .path = vfs::BuildPath("Root"),
        .flags = eOsDeviceCreateNew,
        .interface = kOsFolderGuid,
    };

    ASSERT_EQ(sys::SysDeviceOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    ASSERT_EQ(sys::SysDeviceClose(&i, node), OsStatusSuccess);

    sys::DeviceOpenInfo openInfo2 {
        .path = vfs::BuildPath("Root", "File.txt"),
        .flags = eOsDeviceCreateNew,
        .interface = kOsFileGuid,
    };

    ASSERT_EQ(sys::SysDeviceOpen(&i, openInfo2, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";
}
