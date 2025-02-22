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
#include "std/rcuptr.hpp"

#include "util/absl.hpp"
#include "std/vector.hpp"

namespace km {
    enum class ThreadId : uint64_t { };
    enum class AddressSpaceId : uint64_t { };
    enum class ProcessId : uint64_t { };
    enum class MutexId : uint64_t { };

    template<typename T>
    class IdAllocator {
        std::atomic<std::underlying_type_t<T>> mCurrent = OS_HANDLE_INVALID + 1;

    public:
        T allocate() {
            return T(mCurrent.fetch_add(1));
        }
    };

    struct Thread;
    struct AddressSpace;
    struct Process;
    struct Mutex;

    class SystemObjects;

    class KernelObject : public sm::RcuIntrusivePtr<KernelObject> {
        OsHandle mId;
        stdx::String mName;

    protected:
        KernelObject(OsHandle id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
        { }

    public:
        virtual ~KernelObject() = default;


        OsHandle id() const { return mId; }
        stdx::StringView name() const { return mName; }
    };

    struct Thread : public KernelObject {
        Thread(ThreadId id, stdx::String name, sm::RcuSharedPtr<Process> process)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleThread) << 56), std::move(name))
            , process(process)
        { }

        sm::RcuWeakPtr<Process> process;
        sm::RcuWeakPtr<Mutex> wait;
        km::IsrContext state;
        std::unique_ptr<std::byte[]> stack;
    };

    struct AddressSpace {
        AddressSpaceId id;
        stdx::String name;
        km::AddressMapping mapping;
    };

    struct Process : public KernelObject {
        x64::Privilege privilege;

        Process(ProcessId id, stdx::String name, x64::Privilege privilege)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleProcess) << 56), std::move(name))
            , privilege(privilege)
        { }

        x64::MachineState machine;
        stdx::Vector2<sm::RcuSharedPtr<Thread>> threads;
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
        sm::RcuDomain mDomain;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;

        sm::FlatHashMap<ThreadId, sm::RcuSharedPtr<Thread>> mThreads;
        sm::FlatHashMap<AddressSpaceId, std::unique_ptr<AddressSpace>> mAddressSpaces;
        sm::FlatHashMap<ProcessId, sm::RcuSharedPtr<Process>> mProcesses;
        sm::FlatHashMap<MutexId, std::unique_ptr<Mutex>> mMutexes;

    public:
        SystemObjects() = default;

        sm::RcuSharedPtr<Thread> createThread(stdx::String name, sm::RcuSharedPtr<Process> process);
        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping);
        sm::RcuSharedPtr<Process> createProcess(stdx::String name, x64::Privilege privilege);
        Mutex *createMutex(stdx::String name);

        sm::RcuSharedPtr<Thread> getThread(ThreadId id);
        AddressSpace *getAddressSpace(AddressSpaceId id);
        sm::RcuSharedPtr<Process> getProcess(ProcessId id);
        Mutex *getMutex(MutexId id);
    };

    struct ProcessLaunch {
        sm::RcuSharedPtr<Process> process;
        sm::RcuSharedPtr<Thread> main;
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
