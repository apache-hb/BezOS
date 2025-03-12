#pragma once

#include "process/process.hpp"

namespace vfs2 {
    class VfsRoot;
}

namespace km {
    class SystemObjects {
        stdx::SharedSpinLock mLock;
        SystemMemory *mMemory = nullptr;
        vfs2::VfsRoot *mVfs = nullptr;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;
        IdAllocator<VNodeId> mDeviceIds;

        sm::FlatHashMap<ThreadId, Thread*> mThreads;
        sm::FlatHashMap<ProcessId, Process*> mProcesses;
        sm::FlatHashMap<MutexId, Mutex*> mMutexes;
        sm::FlatHashMap<VNodeId, VNode*> mNodes;

        bool releaseHandle(KernelObject *object);
        void destroyHandle(KernelObject *object);

    public:
        SystemObjects(SystemMemory *memory, vfs2::VfsRoot *vfs)
            : mMemory(memory)
            , mVfs(vfs)
        { }

        OsStatus createProcess(stdx::String name, x64::Privilege privilege, MemoryRange pteMemory, Process **process);
        OsStatus destroyProcess(Process *process);
        Process *getProcess(ProcessId id);
        OsStatus exitProcess(Process *process, int64_t status);

        OsStatus createThread(stdx::String name, Process *process, Thread **thread);
        OsStatus destroyThread(Thread *thread);
        Thread *getThread(ThreadId id);

        OsStatus createVNode(const vfs2::VfsPath &path, sm::uuid uuid, const void *data, size_t size, Process *process, VNode **node);
        OsStatus destroyVNode(Process *process, VNode *node);
        VNode *getVNode(VNodeId id);
        OsStatus addVNode(Process *process, std::unique_ptr<vfs2::IHandle> handle, VNode **node);

        Thread *createThread(stdx::String name, Process* process);
        Mutex *createMutex(stdx::String name);

        Mutex *getMutex(MutexId id);

        KernelObject *getHandle(OsHandle id);
    };
}
