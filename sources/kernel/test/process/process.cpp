#include <gtest/gtest.h>

#include "process/process.hpp"
#include "process/system.hpp"

#include "fs2/vfs.hpp"

#include "test/test_memory.hpp"

TEST(ProcessTest, Construct) {
    vfs2::VfsRoot vfs;
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make();

    km::SystemObjects objects { &memory, &vfs };
}

TEST(ProcessTest, CreateProcess) {
    vfs2::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    km::Process *process = nullptr;
    {
        OsStatus status = objects.createProcess("test", x64::Privilege::eUser, s0, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);

    ASSERT_EQ(process->name(), "test");
    ASSERT_EQ(process->privilege, x64::Privilege::eUser);

    ASSERT_TRUE(process->publicId() != OS_HANDLE_INVALID);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }
}
