#include <gtest/gtest.h>

#include "kernel.hpp"
#include "process/process.hpp"
#include "process/device.hpp"
#include "process/system.hpp"
#include "process/thread.hpp"

#include "fs/vfs.hpp"

#include "test/test_memory.hpp"
#include "xsave.hpp"

km::SystemMemory *km::GetSystemMemory() {
    return nullptr;
}

class ProcessTest : public testing::Test {
public:
    void SetUp() override {
        km::detail::SetupXSave(km::SaveMode::eNoSave, 0);
    }
};

TEST_F(ProcessTest, Construct) {
    vfs::VfsRoot vfs;
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make();

    km::SystemObjects objects { &memory, &vfs };
}

TEST_F(ProcessTest, CreateProcess) {
    vfs::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    km::ProcessCreateInfo createInfo {
        .privilege = x64::Privilege::eUser,
    };
    km::Process *process = nullptr;
    {
        OsStatus status = objects.createProcess("test", s0, createInfo, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);

    ASSERT_EQ(process->getName(), "test");

    ASSERT_TRUE(process->publicId() != OS_HANDLE_INVALID);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }
}

TEST_F(ProcessTest, CreateThread) {
    vfs::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    km::ProcessCreateInfo createInfo {
        .privilege = x64::Privilege::eUser,
    };
    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    {
        OsStatus status = objects.createProcess("test", s0, createInfo, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->getName(), "test");
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->getName(), "test");
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

TEST_F(ProcessTest, OpenFile) {
    vfs::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        OsStatus status = vfs.mkpath(vfs::BuildPath("Test"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        OsStatus status = vfs.create(vfs::BuildPath("Test", "File.txt"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::ProcessCreateInfo createInfo {
        .privilege = x64::Privilege::eUser,
    };
    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    km::Device *vnode = nullptr;
    {
        OsStatus status = objects.createProcess("test", s0, createInfo, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->getName(), "test");
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->getName(), "test");
    ASSERT_EQ(thread->process, process);
    ASSERT_NE(thread->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createDevice(vfs::BuildPath("Test", "File.txt"), kOsFileGuid, nullptr, 0, process, &vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(vnode, nullptr);
    ASSERT_EQ(vnode->getName(), "File.txt");
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
    ASSERT_EQ(objects.getDevice(km::DeviceId(OS_HANDLE_ID(vnodeId))), nullptr);
}

TEST_F(ProcessTest, CloseFileInProcess) {
    vfs::VfsRoot vfs;
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make(0x10000 * 16);

    km::SystemObjects objects { &memory, &vfs };

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        OsStatus status = vfs.mkpath(vfs::BuildPath("Test"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sm::RcuSharedPtr<vfs::INode> node = nullptr;
        OsStatus status = vfs.create(vfs::BuildPath("Test", "File.txt"), &node);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    km::ProcessCreateInfo createInfo {
        .privilege = x64::Privilege::eUser,
    };
    km::Process *process = nullptr;
    km::Thread *thread = nullptr;
    km::Device *vnode = nullptr;
    {
        OsStatus status = objects.createProcess("test", s0, createInfo, &process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(process, nullptr);
    ASSERT_EQ(process->getName(), "test");
    ASSERT_NE(process->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createThread("test", process, &thread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(thread, nullptr);
    ASSERT_EQ(thread->getName(), "test");
    ASSERT_EQ(thread->process, process);
    ASSERT_NE(thread->publicId(), OS_HANDLE_INVALID);

    {
        OsStatus status = objects.createDevice(vfs::BuildPath("Test", "File.txt"), kOsFileGuid, nullptr, 0, process, &vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_NE(vnode, nullptr);
    ASSERT_EQ(vnode->getName(), "File.txt");
    ASSERT_NE(vnode->publicId(), OS_HANDLE_INVALID);

    OsHandle threadId = thread->publicId();
    OsHandle processId = process->publicId();
    OsHandle vnodeId = vnode->publicId();

    ASSERT_NE(process->findHandle(threadId), nullptr);
    ASSERT_NE(process->findHandle(vnodeId), nullptr);

    {
        OsStatus status = objects.destroyDevice(process, vnode);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    ASSERT_EQ(process->findHandle(vnodeId), nullptr);
    ASSERT_EQ(objects.getDevice(km::DeviceId(OS_HANDLE_ID(vnodeId))), nullptr);

    {
        OsStatus status = objects.destroyProcess(process);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    // the thread should be destroyed with the process

    ASSERT_EQ(objects.getProcess(km::ProcessId(OS_HANDLE_ID(processId))), nullptr);
    ASSERT_EQ(objects.getThread(km::ThreadId(OS_HANDLE_ID(threadId))), nullptr);
    ASSERT_EQ(objects.getDevice(km::DeviceId(OS_HANDLE_ID(vnodeId))), nullptr);
}
