#include "fs2/vfs.hpp"
#include "memory.hpp"
#include "system/create.hpp"
#include "system/process.hpp"
#include "system/system.hpp"
#include "system_test.hpp"

using namespace std::string_view_literals;

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

class QuerySystemTest : public SystemBaseTest {
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
    }

    void TearDown() override {
        OsStatus status = sys2::SysDestroyRootProcess(system(), hRootProcess.get());
        ASSERT_EQ(status, OsStatusSuccess);
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys2::ProcessHandle> hRootProcess = nullptr;

    sys2::System *system() { return &data->system; }
};

TEST_F(QuerySystemTest, TransactCreateProcess) {
    OsProcessHandle hChild = OS_HANDLE_INVALID;
    OsThreadHandle hThread = OS_HANDLE_INVALID;

    {
        OsProcessCreateInfo createInfo {
            .Name = "CHILD",
            .Parent = hRootProcess->getHandle(),
        };
        sys2::InvokeContext invoke { system(), hRootProcess->getProcess() };
        OsStatus status = sys2::SysProcessCreate(&invoke, hRootProcess->getProcess(), createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    sm::RcuSharedPtr<sys2::Process> pChild = GetProcess(hRootProcess->getProcess(), hChild);

    {
        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "MAIN",
            .cpuState = {},
            .tlsAddress = 0,
            .kernelStackSize = x64::kPageSize * 8,
        };

        sys2::InvokeContext invoke { system(), pChild };
        OsStatus status = sys2::SysThreadCreate(&invoke, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // create a process and fill out its address space inside a transaction

    OsStatus status = OsStatusSuccess;
    OsProcessHandle hZshProcess = OS_HANDLE_INVALID;
    OsThreadHandle hZshMainThread = OS_HANDLE_INVALID;
    OsProcessCreateInfo processCreateInfo {
        .Name = "ZSH.ELF",
    };

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "MAIN",
        .kernelStackSize = x64::kPageSize * 8,
    };

    {
        sys2::InvokeContext invoke { system(), pChild, GetThread(pChild, hThread) };
        status = sys2::SysProcessCreate(&invoke, processCreateInfo, &hZshProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hZshProcess, OS_HANDLE_INVALID) << "Process was not created";

        status = sys2::SysThreadCreate(&invoke, threadCreateInfo, &hZshMainThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    auto pZshProcess = GetProcess(pChild, hZshProcess);
    auto name = pZshProcess->getName();
    ASSERT_EQ(name.count(), sizeof("ZSH.ELF") - 1) << "Process name was not set correctly";
    ASSERT_EQ(name, "ZSH.ELF"sv) << "Process name was not set correctly '" << std::string_view(name) << "'";

    // ensure that the process is not visible outside the transaction
    {
        sys2::InvokeContext invoke { system(), pChild, GetThread(pChild, hThread) };
        OsProcessHandle handles[1]{};
        sys2::ProcessQueryInfo query {
            .limit = 1,
            .handles = handles,
            .access = eOsProcessAccessStat,
            .matchName = "ZSH.ELF",
        };
        sys2::ProcessQueryResult result{};

        status = sys2::SysQueryProcessList(&invoke, query, &result);
        ASSERT_EQ(status, OsStatusSuccess) << "Failed to query process list";
        ASSERT_EQ(result.found, 1) << "Failed to find process";
    }
}
