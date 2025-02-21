#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "arch/arch.hpp"
#include "fs2/node.hpp"
#include "isr/isr.hpp"
#include "memory/memory.hpp"
#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"
#include "std/string.hpp"

#include "util/absl.hpp"
#include "std/vector.hpp"

namespace km {
    enum class ThreadId : uint64_t { };
    enum class AddressSpaceId : uint64_t { };
    enum class ProcessId : uint64_t { };
    enum class MutexId : uint64_t { };

    template<typename T>
    class IdAllocator {
        T mCurrent = T();

    public:
        T allocate() {
            auto result = mCurrent;
            mCurrent = T(std::to_underlying(mCurrent) + 1);
            return result;
        }
    };

    struct Thread;
    struct AddressSpace;
    struct Process;
    struct Mutex;

    class SystemObjects;

    struct Thread {
        ThreadId id;
        stdx::String name;
        Process *process;
        Mutex *wait;
        km::IsrContext state;
        std::unique_ptr<std::byte[]> stack;
    };

    struct AddressSpace {
        AddressSpaceId id;
        stdx::String name;
        km::AddressMapping mapping;
    };

    struct Process {
        ProcessId id;
        stdx::String name;
        x64::Privilege privilege;
        x64::MachineState machine;
        stdx::Vector2<Thread*> threads;
        stdx::Vector2<AddressSpace*> memory;
        stdx::Vector2<std::unique_ptr<vfs2::IVfsNodeHandle>> files;
    };

    struct Mutex {
        MutexId id;
        stdx::String name;
        stdx::SpinLock lock;

        // TODO: store a list of all the threads waiting on this mutex
    };

    class SystemObjects {
        stdx::SharedSpinLock mLock;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;

        sm::FlatHashMap<ThreadId, std::unique_ptr<Thread>> mThreads;
        sm::FlatHashMap<AddressSpaceId, std::unique_ptr<AddressSpace>> mAddressSpaces;
        sm::FlatHashMap<ProcessId, std::unique_ptr<Process>> mProcesses;
        sm::FlatHashMap<MutexId, std::unique_ptr<Mutex>> mMutexes;

    public:
        SystemObjects() = default;

        Thread *createThread(stdx::String name, Process *process);
        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping);
        Process *createProcess(stdx::String name, x64::Privilege privilege);
        Mutex *createMutex(stdx::String name);

        Thread *getThread(ThreadId id);
        AddressSpace *getAddressSpace(AddressSpaceId id);
        Process *getProcess(ProcessId id);
        Mutex *getMutex(MutexId id);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };

    OsStatus SysProcessCreate(OsProcessCreateInfo createInfo, OsProcessHandle *outHandle);
    OsStatus SysProcessCurrent(OsProcessHandle *outHandle);
    OsStatus SysProcessSuspend(OsProcessHandle handle, bool suspend);
    OsStatus SysProcessTerminate(OsProcessHandle handle);

    OsStatus SysThreadCreate(OsThreadCreateInfo createInfo, OsThreadHandle *outHandle);
    OsStatus SysThreadDestroy(OsThreadHandle handle);
    OsStatus SysThreadCurrent(OsThreadHandle *outHandle);
    OsStatus SysThreadSleep(OsDuration duration);
    OsStatus SysThreadSuspend(OsThreadHandle handle, bool suspend);

    OsStatus SysMutexCreate(OsMutexCreateInfo createInfo, OsMutexHandle *outHandle);
    OsStatus SysMutexDestroy(OsMutexHandle handle);
    OsStatus SysMutexLock(OsMutexHandle handle);
    OsStatus SysMutexUnlock(OsMutexHandle handle);
}
