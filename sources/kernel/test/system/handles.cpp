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

class HandleTest : public SystemBaseTest {
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
        return sys2::InvokeContext { system(), GetProcess(hRootProcess->getProcess(), hProcess), OS_HANDLE_INVALID };
    }
};

TEST_F(HandleTest, CloneHandle) {
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs2::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs2::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs2::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys2::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys2::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    OsNodeInfo info{};
    ASSERT_EQ(sys2::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    ASSERT_EQ(sys2::SysHandleClose(&i, node), OsStatusSuccess) << "Node handle was not closed";
    ASSERT_EQ(sys2::SysHandleClose(&i, node2), OsStatusSuccess) << "Node handle was not closed";
}

TEST_F(HandleTest, StatHandle) {
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs2::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs2::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs2::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys2::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys2::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    OsNodeInfo info{};
    ASSERT_EQ(sys2::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys2::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}

TEST_F(HandleTest, HandleClose) {
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs2::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs2::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs2::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys2::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys2::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    ASSERT_EQ(sys2::SysHandleClose(&i, node), OsStatusSuccess) << "Node handle was not closed";

    OsNodeInfo info{};
    ASSERT_EQ(sys2::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys2::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}

TEST_F(HandleTest, NodeQuery) {
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs2::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs2::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs2::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys2::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsDeviceHandle device = OS_HANDLE_INVALID;
    OsNodeQueryInterfaceInfo query {
        .InterfaceGuid = kOsFileGuid,
    };
    ASSERT_EQ(sys2::SysNodeQuery(&i, node, query, &device), OsStatusSuccess);

    ASSERT_NE(device, OS_HANDLE_INVALID) << "Device handle was not created";
}

TEST_F(HandleTest, NodeClose) {
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    ASSERT_EQ(data->vfs.mkpath(vfs2::BuildPath("Root"), &vfsNode), OsStatusSuccess);
    ASSERT_EQ(data->vfs.create(vfs2::BuildPath("Root", "File.txt"), &vfsNode), OsStatusSuccess);

    auto i = invoke();
    OsNodeHandle node = OS_HANDLE_INVALID;
    sys2::NodeOpenInfo openInfo {
        .process = GetProcessHandle(hRootProcess->getProcess(), hProcess),
        .path = vfs2::BuildPath("Root", "File.txt"),
    };
    ASSERT_EQ(sys2::SysNodeOpen(&i, openInfo, &node), OsStatusSuccess);
    ASSERT_NE(node, OS_HANDLE_INVALID) << "Node handle was not created";

    OsNodeHandle node2 = OS_HANDLE_INVALID;
    OsHandleCloneInfo cloneInfo { .Access = eOsNodeAccessStat };
    ASSERT_EQ(sys2::SysHandleClone(&i, node, cloneInfo, &node2), OsStatusSuccess);

    ASSERT_NE(node2, OS_HANDLE_INVALID) << "Node handle was not cloned";

    ASSERT_EQ(sys2::SysNodeClose(&i, node), OsStatusSuccess) << "Node handle was not closed";

    OsNodeInfo info{};
    ASSERT_EQ(sys2::SysNodeStat(&i, node2, &info), OsStatusSuccess);
    ASSERT_STREQ(info.Name, "File.txt") << "Node name was not set correctly";

    OsHandleInfo handleInfo{};
    ASSERT_EQ(sys2::SysHandleStat(&i, node2, &handleInfo), OsStatusSuccess);
    ASSERT_EQ(handleInfo.Access, eOsNodeAccessStat) << "Handle type was not set correctly";
}
