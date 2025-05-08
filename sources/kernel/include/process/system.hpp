#pragma once

#include "process/process.hpp"

#include "fs/path.hpp"
#include "std/shared_spinlock.hpp"
#include "util/uuid.hpp"

namespace vfs {
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
        vfs::VfsRoot *mVfs = nullptr;

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

        sm::FlatHashMap<sm::RcuSharedPtr<vfs::INode>, BaseObject*, sm::RcuHash<vfs::INode>> mVfsNodes;
        sm::FlatHashMap<vfs::IHandle*, BaseObject*> mVfsHandles;

        bool releaseHandle(BaseObject *object);
        void destroyHandle(BaseObject *object);

    public:
        SystemObjects(SystemMemory *memory, vfs::VfsRoot *vfs)
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

        OsStatus createNode(Process *process, sm::RcuSharedPtr<vfs::INode> vfsNode, Node **result);
        OsStatus destroyNode(Process *process, Node *node);
        Node *getNode(NodeId id);

        OsStatus createDevice(const vfs::VfsPath &path, sm::uuid uuid, const void *data, size_t size, Process *process, Device **node);
        OsStatus addDevice(Process *process, std::unique_ptr<vfs::IHandle> handle, Device **node);
        OsStatus destroyDevice(Process *process, Device *node);
        Device *getDevice(DeviceId id);

        OsHandle getNodeId(sm::RcuSharedPtr<vfs::INode> node);
        OsHandle getHandleId(vfs::IHandle *handle);

        Thread *createThread(stdx::String name, Process* process);
        Mutex *createMutex(stdx::String name);

        Mutex *getMutex(MutexId id);

        BaseObject *getHandle(OsHandle id);
    };
}
