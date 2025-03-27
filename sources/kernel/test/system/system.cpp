#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "test/test_memory.hpp"

class SystemTest : public testing::Test {
public:
    void SetUp() override {
        body.addSegment(0x100000, boot::MemoryRegion::eUsable);
        body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    }

    SystemMemoryTestBody body;
};

TEST_F(SystemTest, Construct) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(1, 1);
    sys2::System system(&schedule, &memory.systemTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, ConstructMultiple) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.systemTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, CreateProcess) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.systemTables(), &memory.pmmAllocator());

    sm::RcuSharedPtr<sys2::Process> process;
    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    OsStatus status = sys2::CreateProcess(&system, createInfo, &process);
    ASSERT_EQ(status, OsStatusSuccess);
}
