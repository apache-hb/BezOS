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
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;
        IdAllocator<DeviceId> mDeviceIds;

        sm::FlatHashMap<ThreadId, Thread*> mThreads;
        sm::FlatHashMap<AddressSpaceId, AddressSpace*> mAddressSpaces;
        sm::FlatHashMap<ProcessId, Process*> mProcesses;
        sm::FlatHashMap<MutexId, Mutex*> mMutexes;
        sm::FlatHashMap<DeviceId, VNode*> mNodes;

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

        Thread *createThread(stdx::String name, Process* process);
        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping, km::PageFlags flags, km::MemoryType type, Process* process);
        Mutex *createMutex(stdx::String name);

        AddressSpace *getAddressSpace(AddressSpaceId id);
        Mutex *getMutex(MutexId id);
    };
}
