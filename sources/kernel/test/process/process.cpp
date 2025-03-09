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

TEST(ProcessTest, CreateThread) {
    vfs2::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    {
        OsStatus status = objects.createProcess("test", x64::Privilege::eUser, s0, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->name(), "test");
    ASSERT_EQ(process->privilege, x64::Privilege::eUser);
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->name(), "test");
    ASSERT_EQ(thread->process, process);
    ASSERT_NE(thread->publicId(), OS_HANDLE_INVALID);

    OsHandle threadId = thread->publicId();
    OsHandle processId = process->publicId();

    ASSERT_NE(process->findHandle(thread->publicId()), nullptr);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // the thread should be destroyed with the process

    ASSERT_EQ(objects.getProcess(km::ProcessId(OS_HANDLE_ID(processId))), nullptr);
    ASSERT_EQ(objects.getThread(km::ThreadId(OS_HANDLE_ID(threadId))), nullptr);
}

TEST(ProcessTest, OpenFile) {
    vfs2::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    {
        vfs2::IVfsNode *node = nullptr;
        OsStatus status = vfs.mkpath(vfs2::BuildPath("Test"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        vfs2::IVfsNode *node = nullptr;
        OsStatus status = vfs.create(vfs2::BuildPath("Test", "File.txt"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    km::VNode *vnode = nullptr;
    {
        OsStatus status = objects.createProcess("test", x64::Privilege::eUser, s0, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->name(), "test");
    ASSERT_EQ(process->privilege, x64::Privilege::eUser);
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->name(), "test");
    ASSERT_EQ(thread->process, process);
    ASSERT_NE(thread->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createVNode(vfs2::BuildPath("Test", "File.txt"), process, &vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(vnode, nullptr);
    ASSERT_EQ(vnode->name(), "File.txt");
    ASSERT_NE(vnode->publicId(), OS_HANDLE_INVALID);

    OsHandle threadId = thread->publicId();
    OsHandle processId = process->publicId();
    OsHandle vnodeId = vnode->publicId();

    ASSERT_NE(process->findHandle(threadId), nullptr);
    ASSERT_NE(process->findHandle(vnodeId), nullptr);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // the thread should be destroyed with the process

    ASSERT_EQ(objects.getProcess(km::ProcessId(OS_HANDLE_ID(processId))), nullptr);
    ASSERT_EQ(objects.getThread(km::ThreadId(OS_HANDLE_ID(threadId))), nullptr);
    ASSERT_EQ(objects.getVNode(km::VNodeId(OS_HANDLE_ID(vnodeId))), nullptr);
}

TEST(ProcessTest, CloseFileInProcess) {
    vfs2::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    {
        vfs2::IVfsNode *node = nullptr;
        OsStatus status = vfs.mkpath(vfs2::BuildPath("Test"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        vfs2::IVfsNode *node = nullptr;
        OsStatus status = vfs.create(vfs2::BuildPath("Test", "File.txt"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    km::VNode *vnode = nullptr;
    {
        OsStatus status = objects.createProcess("test", x64::Privilege::eUser, s0, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->name(), "test");
    ASSERT_EQ(process->privilege, x64::Privilege::eUser);
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->name(), "test");
    ASSERT_EQ(thread->process, process);
    ASSERT_NE(thread->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createVNode(vfs2::BuildPath("Test", "File.txt"), process, &vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(vnode, nullptr);
    ASSERT_EQ(vnode->name(), "File.txt");
    ASSERT_NE(vnode->publicId(), OS_HANDLE_INVALID);

    OsHandle threadId = thread->publicId();
    OsHandle processId = process->publicId();
    OsHandle vnodeId = vnode->publicId();

    ASSERT_NE(process->findHandle(threadId), nullptr);
    ASSERT_NE(process->findHandle(vnodeId), nullptr);

    {
        OsStatus status = objects.destroyVNode(process, vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_EQ(process->findHandle(vnodeId), nullptr);
    ASSERT_EQ(objects.getVNode(km::VNodeId(OS_HANDLE_ID(vnodeId))), nullptr);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // the thread should be destroyed with the process

    ASSERT_EQ(objects.getProcess(km::ProcessId(OS_HANDLE_ID(processId))), nullptr);
    ASSERT_EQ(objects.getThread(km::ThreadId(OS_HANDLE_ID(threadId))), nullptr);
    ASSERT_EQ(objects.getVNode(km::VNodeId(OS_HANDLE_ID(vnodeId))), nullptr);
}
