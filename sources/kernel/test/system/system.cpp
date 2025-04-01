#include <barrier>
#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "system/thread.hpp"
#include "test/test_memory.hpp"

#include <thread>

struct MemoryState {
    km::PageAllocatorStats pmm;
    km::PteAllocatorStats vmm;
    size_t rangeStats;
};

class SystemTest : public testing::Test {
public:
    void SetUp() override {
        km::detail::SetupXSave(km::SaveMode::eNoSave, 0);

        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
    }

    MemoryState GetMemoryState(km::SystemMemory& memory) {
        MemoryState state;
        state.pmm = memory.pmmAllocator().stats();
        state.vmm = memory.systemTables().TESTING_getPageTableAllocator().stats();
        state.rangeStats = memory.pageTables().TESTING_getVmemAllocator().freeSpace();
        return state;
    }

    void CheckMemoryState(km::SystemMemory& memory, const MemoryState& before) {
        (void)memory.systemTables().compact();
        auto after = GetMemoryState(memory);

        ASSERT_EQ(after.vmm.freeBlocks, before.vmm.freeBlocks) << "Virtual memory was leaked";
        ASSERT_EQ(after.pmm.freeMemory, before.pmm.freeMemory) << "Physical memory was leaked";
        ASSERT_EQ(after.rangeStats, before.rangeStats) << "Address space memory was leaked";
    }

    void RecordMemoryUsage(km::SystemMemory& memory) {
        before = GetMemoryState(memory);
    }

    void CheckMemoryUsage(km::SystemMemory& memory) {
        (void)memory.systemTables().compact();

        CheckMemoryState(memory, before);
    }

    void DebugMemoryUsage(km::SystemMemory& memory) {
        auto state = GetMemoryState(memory);

        std::cout << "PMM: " << state.pmm.freeMemory << std::endl;
        std::cout << "VMM: " << state.vmm.freeBlocks << std::endl;
        std::cout << "Address Space: " << std::string_view(km::format(sm::bytes(state.rangeStats))) << std::endl;
    }

    SystemMemoryTestBody body { sm::megabytes(4).bytes() };

    MemoryState before;
    MemoryState after;
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
        auto process = hProcess->getProcessObject();
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

TEST_F(SystemTest, NestedChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(8).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ProcessHandle *hChild = hProcess.get();
        for (size_t i = 0; i < 8; i++) {
            sys2::ProcessCreateInfo createInfo {
                .name = "CHILD",
                .supervisor = false,
            };

            DebugMemoryUsage(memory);

            OsStatus status = hChild->createProcess(&system, createInfo, &hChild);
            ASSERT_EQ(status, OsStatusSuccess);

            ASSERT_NE(hChild, nullptr) << "Child process was not created";
        }
    }

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    OsStatus status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}


TEST_F(SystemTest, OrphanThread) {
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
    auto process = hProcess->getProcessObject();

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = hProcess->destroyProcess(&system, destroyInfo);
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

    auto before0 = GetMemoryState(memory);

    OsStatus status = sys2::CreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    auto before1 = GetMemoryState(memory);

    status = hProcess->createThread(&system, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, nullptr) << "Thread was not created";
    auto process = hProcess->getProcessObject();

    status = hThread->destroy(&system, sys2::ThreadDestroyInfo { .exitCode = 0, .reason = eOsThreadFinished });
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before1);

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = hProcess->destroyProcess(&system, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before0);
}
