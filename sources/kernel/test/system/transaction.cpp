#include "system/transaction.hpp"
#include "memory.hpp"
#include "system/process.hpp"
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
            .exitCode = 0,
            .reason = eOsProcessExited,
        };

        OsStatus status = hRootProcess->destroyProcess(system(), destroyInfo);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    std::unique_ptr<TestData> data;
    std::unique_ptr<sys2::ProcessHandle> hRootProcess = nullptr;

    sys2::System *system() { return &data->system; }
};

TEST_F(TxSystemTest, TransactCreateProcess) {
    sys2::ProcessHandle *child = nullptr;
    sys2::ThreadHandle *hThread = nullptr;
    sys2::TxHandle *hTx = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "CHILD",
        };
        OsStatus status = hRootProcess->createProcess(system(), createInfo, &child);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "MAIN",
            .kernelStackSize = x64::kPageSize * 8,
        };

        OsStatus status = child->createThread(system(), threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::TxCreateInfo txCreateInfo {
            .name = "TEST",
        };

        OsStatus status = child->createTx(system(), txCreateInfo, &hTx);
        ASSERT_EQ(status, OsStatusSuccess);
        ASSERT_NE(hTx, nullptr) << "Transaction was not created";
    }

    // create a process and fill out its address space inside a transaction

    OsStatus status = OsStatusSuccess;
    sys2::ProcessHandle *hZshProcess = nullptr;
    sys2::ThreadHandle *hZshMainThread = nullptr;
    sys2::ProcessCreateInfo processCreateInfo {
        .name = "ZSH.ELF",
    };
    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "MAIN",
        .kernelStackSize = x64::kPageSize * 8,
    };

    status = hTx->createProcess(system(), processCreateInfo, &hZshProcess);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hZshProcess, nullptr) << "Process was not created";

    status = hTx->createThread(system(), hZshProcess, threadCreateInfo, &hZshMainThread);
    ASSERT_EQ(status, OsStatusSuccess);

#if 0
    // ensure that the process is not visible outside the transaction
    sys2::ProcessHandle *hFindProcess = nullptr;
    status = system()->getProcessByName(processCreateInfo.name, &hFindProcess);
    ASSERT_EQ(status, OsStatusNotFound) << "Process was found outside the transaction";
    ASSERT_EQ(hFindProcess, nullptr) << "Process was found outside the transaction";
#endif
}
