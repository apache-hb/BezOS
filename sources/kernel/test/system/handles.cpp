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

class HandleTest : public SystemBaseTest {
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

TEST_F(HandleTest, CloneHandle) {
    sm::RcuSharedPtr<vfs::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    OsNodeInfo info{};
    ASSERT_EQ(sys::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    ASSERT_EQ(sys::SysHandleClose(&i, node), OsStatusSuccess) << "Node handle was not closed";
    ASSERT_EQ(sys::SysHandleClose(&i, node2), OsStatusSuccess) << "Node handle was not closed";
}

TEST_F(HandleTest, StatHandle) {
    sm::RcuSharedPtr<vfs::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    OsNodeInfo info{};
    ASSERT_EQ(sys::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}

TEST_F(HandleTest, HandleClose) {
    sm::RcuSharedPtr<vfs::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    ASSERT_EQ(sys::SysHandleClose(&i, node), OsStatusSuccess) << "Node handle was not closed";

    OsNodeInfo info{};
    ASSERT_EQ(sys::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}

TEST_F(HandleTest, NodeQuery) {
    sm::RcuSharedPtr<vfs::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsDeviceHandle device = OS_HANDLE_INVALID;
    OsNodeQueryInterfaceInfo query {
        .InterfaceGuid = kOsFileGuid,
    };
    ASSERT_EQ(sys::SysNodeQuery(&i, node, query, &device), OsStatusSuccess);

    ASSERT_NE(device, OS_HANDLE_INVALID) << "Device handle was not created";
}

TEST_F(HandleTest, NodeClose) {
    sm::RcuSharedPtr<vfs::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    ASSERT_EQ(sys::SysNodeClose(&i, node), OsStatusSuccess) << "Node handle was not closed";

    OsNodeInfo info{};
    ASSERT_EQ(sys::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}
