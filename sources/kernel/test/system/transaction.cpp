#include "system/transaction.hpp"
#include "fs2/vfs.hpp"
#include "memory.hpp"
#include "system/create.hpp"
#include "system/process.hpp"
#include "system/system.hpp"
#include "system_test.hpp"

struct TestData {
    km::SystemMemory memory;
    vfs2::VfsRoot vfs;
    sys2::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(2).bytes()))
        , system({ 128, 128 }, &memory.pageTables(), &memory.pmmAllocator(), &vfs)
    { }
};

class TxSystemTest : public SystemBaseTest {
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

TEST_F(TxSystemTest, TransactCreateProcess) {
    OsProcessHandle hChild = OS_HANDLE_INVALID;
    OsThreadHandle hThread = OS_HANDLE_INVALID;
    OsTxHandle hTx = OS_HANDLE_INVALID;

    {
        OsProcessCreateInfo createInfo {
            .Name = "CHILD",
        };

        sys2::InvokeContext invoke { system(), hRootProcess->getProcess(), OS_HANDLE_INVALID };
        OsStatus status = sys2::SysCreateProcess(&invoke, hRootProcess->getProcess(), createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "MAIN",
            .cpuState = {},
            .tlsAddress = 0,
            .kernelStackSize = x64::kPageSize * 8,
        };

        sys2::InvokeContext invoke { system(), hRootProcess->getProcess(), OS_HANDLE_INVALID };
        OsStatus status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::TxCreateInfo txCreateInfo {
            .name = "TEST",
        };

        sys2::InvokeContext invoke { system(), GetProcess(hRootProcess->getProcess(), hChild), OS_HANDLE_INVALID };
        OsStatus status = sys2::SysCreateTx(&invoke, txCreateInfo, &hTx);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hTx, OS_HANDLE_INVALID) << "Transaction was not created";
    }

    // create a process and fill out its address space inside a transaction

    OsStatus status = OsStatusSuccess;
    OsProcessHandle hZshProcess = OS_HANDLE_INVALID;
    OsThreadHandle hZshMainThread = OS_HANDLE_INVALID;
    OsProcessCreateInfo processCreateInfo {
        .Name = "ZSH.ELF",
    };
    auto child = GetProcess(hRootProcess->getProcess(), hChild);

    {
        sys2::InvokeContext invoke { system(), child, hThread, hTx };
        status = sys2::SysCreateProcess(&invoke, processCreateInfo, &hZshProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hZshProcess, OS_HANDLE_INVALID) << "Process was not created";

        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "MAIN",
            .kernelStackSize = x64::kPageSize * 8,
        };

        sys2::InvokeContext invoke2 { system(), GetProcess(child, hZshProcess), hThread, hTx };
        status = sys2::SysCreateThread(&invoke2, threadCreateInfo, &hZshMainThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_EQ(GetProcess(child, hZshProcess)->getName(), "ZSH.ELF") << "Process name was not set correctly";

    // ensure that the process is not visible outside the transaction
    {
        sys2::InvokeContext invoke { system(), GetProcess(hRootProcess->getProcess(), hChild), hThread };
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
        ASSERT_EQ(result.found, 0) << "Process was found outside the transaction";
    }
}
