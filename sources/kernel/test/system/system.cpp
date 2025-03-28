#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "test/test_memory.hpp"

class SystemTest : public testing::Test {
public:
    void SetUp() override {
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
    }

    SystemMemoryTestBody body;
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

    auto before = memory.systemTables().TESTING_getPageTableAllocator().stats();

    OsStatus status = sys2::CreateProcess(&system, createInfo, &process);
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::ProcessDestroyInfo destroyInfo {
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = sys2::DestroyProcess(&system, destroyInfo, process);
    ASSERT_EQ(status, OsStatusSuccess);

    (void)memory.systemTables().compact();

    auto after = memory.systemTables().TESTING_getPageTableAllocator().stats();
    ASSERT_EQ(after.freeBlocks, before.freeBlocks) << "Memory was leaked";
}
