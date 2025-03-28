#include <barrier>
#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

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

    sm::RcuSharedPtr<sys2::Process> process;
    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateProcess(&system, createInfo, &process);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = sys2::DestroyProcess(&system, destroyInfo, process);
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

    threads.emplace_back([&](std::stop_token stop) {
        barrier.arrive_and_wait();

        while (!stop.stop_requested()) {
            system.rcuDomain().synchronize();
        }
    });

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&system, &barrier]() {
            barrier.arrive_and_wait();

            for (size_t j = 0; j < kProcessCount; j++) {
                sm::RcuSharedPtr<sys2::Process> process;
                sys2::ProcessCreateInfo createInfo {
                    .name = "TEST",
                    .supervisor = false,
                };

                OsStatus status = sys2::CreateProcess(&system, createInfo, &process);
                if (status == OsStatusOutOfMemory) continue;
                ASSERT_EQ(status, OsStatusSuccess);

                sys2::ProcessDestroyInfo destroyInfo {
                    .exitCode = 0,
                    .reason = eOsProcessExited,
                };

                status = sys2::DestroyProcess(&system, destroyInfo, process);
                ASSERT_EQ(status, OsStatusSuccess);
            }
        });
    }

    threads.clear();

    CheckMemoryUsage(memory);
}
