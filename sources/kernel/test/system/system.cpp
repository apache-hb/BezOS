#include <barrier>
#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "system/thread.hpp"
#include "test/test_memory.hpp"

#include <thread>

class SystemTest : public testing::Test {
public:
    void SetUp() override {
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
    }

    void RecordMemoryUsage(km::SystemMemory& memory) {
        beforePmem = memory.pmmAllocator().stats();
        beforeVmem = memory.systemTables().TESTING_getPageTableAllocator().stats();
    }

    void CheckMemoryUsage(km::SystemMemory& memory) {
        (void)memory.systemTables().compact();

        afterPmem = memory.pmmAllocator().stats();
        afterVmem = memory.systemTables().TESTING_getPageTableAllocator().stats();

        ASSERT_EQ(afterVmem.freeBlocks, beforeVmem.freeBlocks) << "Virtual memory was leaked";
        ASSERT_EQ(afterPmem.freeMemory, beforePmem.freeMemory) << "Physical memory was leaked";
    }

    SystemMemoryTestBody body;

    km::PageAllocatorStats beforePmem;
    km::PteAllocatorStats beforeVmem;
    km::PageAllocatorStats afterPmem;
    km::PteAllocatorStats afterVmem;
};

TEST_F(SystemTest, Construct) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(1, 1);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, ConstructMultiple) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, CreateProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateProcessAsync) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kProcessCount = 128;
    std::barrier barrier(kThreadCount + 1);

    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    threads.emplace_back([&](std::stop_token stop) {
        barrier.arrive_and_wait();

        while (!stop.stop_requested()) {
            system.rcuDomain().synchronize();
        }
    });

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&]() {
            barrier.arrive_and_wait();

            for (size_t j = 0; j < kProcessCount; j++) {
                sys2::ProcessHandle *hChild = nullptr;
                sys2::ProcessCreateInfo createInfo {
                    .name = "CHILD",
                    .supervisor = false,
                };

                OsStatus status = hProcess->createProcess(&system, createInfo, &hChild);
                if (status == OsStatusOutOfMemory) continue;
                ASSERT_EQ(status, OsStatusSuccess);

                sys2::ProcessDestroyInfo destroyInfo {
                    .exitCode = 0,
                    .reason = eOsProcessExited,
                };

                status = hChild->destroyProcess(&system, destroyInfo);
                ASSERT_EQ(status, OsStatusSuccess);
            }
        });
    }

    threads.clear();

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ProcessHandle *hChild = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        RecordMemoryUsage(memory);

        OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "CHILD",
            .supervisor = false,
        };

        OsStatus status = hProcess->createProcess(&system, createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_NE(hChild, nullptr) << "Child process was not created";
        auto process = hProcess->getProcessObject().lock();
        ASSERT_NE(process, nullptr) << "Parent process was not created";

        ASSERT_TRUE(process->getHandle(hChild->getHandle())) << "Parent process does not have child process";
    }

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    OsStatus status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateThread) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    sys2::ProcessCreateInfo processCreateInfo {
        .name = "TEST",
        .supervisor = false,
    };

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ThreadHandle *hThread = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    status = hProcess->createThread(&system, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, nullptr) << "Thread was not created";
    auto process = hProcess->getProcessObject().lock();

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}
