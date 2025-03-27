#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

TEST(SystemTest, Construct) {
    sys2::GlobalSchedule schedule(1, 1);
    sys2::System system(&schedule);
}

TEST(SystemTest, ConstructMultiple) {
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule);
}

TEST(SystemTest, CreateProcess) {
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule);

    sm::RcuSharedPtr<sys2::Process> process;
    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    OsStatus status = sys2::CreateProcess(&system, createInfo, &process);
    ASSERT_EQ(status, OsStatusSuccess);
}
