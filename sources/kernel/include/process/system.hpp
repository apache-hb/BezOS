#pragma once

#include "process/process.hpp"

#include "fs2/path.hpp"
#include "std/shared_spinlock.hpp"
#include "util/uuid.hpp"

namespace vfs2 {
    class VfsRoot;
    class INode;
    class IHandle;
}

namespace km {
    struct Process;
    struct Thread;
    struct Mutex;
    struct Node;
    struct Device;

    class SystemObjects {
        stdx::SharedSpinLock mLock;
        SystemMemory *mMemory = nullptr;
        vfs2::VfsRoot *mVfs = nullptr;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;
        IdAllocator<NodeId> mNodeIds;
        IdAllocator<DeviceId> mDeviceIds;

        sm::FlatHashMap<ThreadId, Thread*> mThreads;
        sm::FlatHashMap<ProcessId, Process*> mProcesses;
        sm::FlatHashMap<MutexId, Mutex*> mMutexes;
        sm::FlatHashMap<NodeId, Node*> mNodes;
        sm::FlatHashMap<DeviceId, Device*> mDevices;

        sm::FlatHashMap<sm::RcuSharedPtr<vfs2::INode>, KernelObject*, sm::RcuHash<vfs2::INode>> mVfsNodes;
        sm::FlatHashMap<vfs2::IHandle*, KernelObject*> mVfsHandles;

        bool releaseHandle(KernelObject *object);
        void destroyHandle(KernelObject *object);

    public:
        SystemObjects(SystemMemory *memory, vfs2::VfsRoot *vfs)
            : mMemory(memory)
            , mVfs(vfs)
        { }

        OsStatus createProcess(stdx::String name, MemoryRange pteMemory, ProcessCreateInfo createInfo, Process **process);
        OsStatus destroyProcess(Process *process);
        Process *getProcess(ProcessId id);
        OsStatus exitProcess(Process *process, int64_t status);

        OsStatus createThread(stdx::String name, Process *process, Thread **thread);
        OsStatus destroyThread(Thread *thread);
        Thread *getThread(ThreadId id);

        OsStatus createNode(Process *process, sm::RcuSharedPtr<vfs2::INode> vfsNode, Node **result);
        OsStatus destroyNode(Process *process, Node *node);
        Node *getNode(NodeId id);

        OsStatus createDevice(const vfs2::VfsPath &path, sm::uuid uuid, const void *data, size_t size, Process *process, Device **node);
        OsStatus addDevice(Process *process, std::unique_ptr<vfs2::IHandle> handle, Device **node);
        OsStatus destroyDevice(Process *process, Device *node);
        Device *getDevice(DeviceId id);

        OsHandle getNodeId(sm::RcuSharedPtr<vfs2::INode> node);
        OsHandle getHandleId(vfs2::IHandle *handle);

        Thread *createThread(stdx::String name, Process* process);
        Mutex *createMutex(stdx::String name);

        Mutex *getMutex(MutexId id);

        KernelObject *getHandle(OsHandle id);
    };
}
