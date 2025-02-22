#include <gtest/gtest.h>

#include "process/process.hpp"

TEST(ProcessTest, Construct) {
    km::SystemObjects objects;
}

TEST(ProcessTest, CreateProcess) {
    km::SystemObjects objects;
    auto process = objects.createProcess("test", x64::Privilege::eUser);
    ASSERT_EQ(process->name(), "test");
    ASSERT_NE(process->id(), OS_HANDLE_INVALID);
}
