#include "system/transaction.hpp"
#include "memory.hpp"
#include "system/create.hpp"
#include "system/process.hpp"
#include "system/system.hpp"
#include "system_test.hpp"

struct TestData {
    km::SystemMemory memory;
    sys2::GlobalSchedule schedule;
    sys2::System system;

    TestData(SystemMemoryTestBody& body)
        : memory(body.make(sm::megabytes(2).bytes()))
        , schedule(128, 128)
        , system(&schedule, &memory.pageTables(), &memory.pmmAllocator())
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

        OsStatus status = sys2::CreateRootProcess(system(), createInfo, std::out_ptr(hRootProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    void TearDown() override {
        sys2::ProcessDestroyInfo destroyInfo {
            .object = hRootProcess.get(),
            .exitCode = 0,
            .reason = eOsProcessExited,
        };
        sys2::InvokeContext invoke { system(), hRootProcess.get(), nullptr };

        OsStatus status = sys2::SysDestroyProcess(&invoke, destroyInfo);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys2::ProcessHandle> hRootProcess = nullptr;

    sys2::System *system() { return &data->system; }
};

TEST_F(TxSystemTest, TransactCreateProcess) {
    sys2::ProcessHandle *hChild = nullptr;
    sys2::ThreadHandle *hThread = nullptr;
    sys2::TxHandle *hTx = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "CHILD",
            .process = hRootProcess.get(),
        };
        sys2::InvokeContext invoke { system(), hRootProcess.get(), nullptr };
        OsStatus status = sys2::SysCreateProcess(&invoke, createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "MAIN",
            .process = hChild,
            .cpuState = {},
            .tlsAddress = 0,
            .kernelStackSize = x64::kPageSize * 8,
        };

        sys2::InvokeContext invoke { system(), hRootProcess.get(), nullptr };
        OsStatus status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::TxCreateInfo txCreateInfo {
            .name = "TEST",
            .process = hChild,
        };

        sys2::InvokeContext invoke { system(), hChild, nullptr };
        OsStatus status = sys2::SysCreateTx(&invoke, txCreateInfo, &hTx);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hTx, nullptr) << "Transaction was not created";
    }

    // create a process and fill out its address space inside a transaction

    OsStatus status = OsStatusSuccess;
    sys2::ProcessHandle *hZshProcess = nullptr;
    sys2::ThreadHandle *hZshMainThread = nullptr;
    sys2::ProcessCreateInfo processCreateInfo {
        .name = "ZSH.ELF",
        .process = hChild,
        .tx = hTx,
    };

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "MAIN",
        .process = hChild,
        .tx = hTx,
        .kernelStackSize = x64::kPageSize * 8,
    };

    {
        sys2::InvokeContext invoke { system(), hChild, hThread };
        status = sys2::SysCreateProcess(&invoke, processCreateInfo, &hZshProcess);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hZshProcess, nullptr) << "Process was not created";

        status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hZshMainThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // ensure that the process is not visible outside the transaction
    sys2::ProcessHandle *hFindProcess = nullptr;

    {
        sys2::InvokeContext invoke { system(), hChild, hThread };
        status = sys2::FindProcessByName(system(), processCreateInfo.name, &hFindProcess);
        status = system()->getProcessByName(processCreateInfo.name, &hFindProcess);
        ASSERT_EQ(status, OsStatusNotFound) << "Process was found outside the transaction";
        ASSERT_EQ(hFindProcess, nullptr) << "Process was found outside the transaction";
    }
}
