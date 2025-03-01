#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "util/defer.hpp"
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
        uint64_t tlsAddress;
        sm::RcuSharedPtr<AddressSpace> stack;
    };

    struct AddressSpace : public KernelObject {
        km::AddressMapping mapping;
        km::PageFlags flags;
        km::MemoryType type;

        AddressSpace(AddressSpaceId id, stdx::String name, km::AddressMapping mapping, km::PageFlags flags, km::MemoryType type)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleAddressSpace) << 56), std::move(name))
            , mapping(mapping)
            , flags(flags)
            , type(type)
        { }
    };

    struct Process : public KernelObject {
        x64::Privilege privilege;
        stdx::SharedSpinLock lock;

        Process(ProcessId id, stdx::String name, x64::Privilege privilege, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleProcess) << 56), std::move(name))
            , privilege(privilege)
            , ptes(kernel, pteMemory, processArea)
        { }

        ProcessPageTables ptes;
        stdx::Vector2<sm::RcuSharedPtr<Thread>> threads;
        stdx::Vector2<sm::RcuSharedPtr<AddressSpace>> memory;
        sm::FlatHashSet<std::unique_ptr<vfs2::IVfsNodeHandle>> files;

        void addThread(sm::RcuSharedPtr<Thread> thread) {
            stdx::UniqueLock guard(lock);
            threads.add(thread);
        }

        void addAddressSpace(sm::RcuSharedPtr<AddressSpace> addressSpace) {
            stdx::UniqueLock guard(lock);
            memory.add(addressSpace);
        }

        vfs2::IVfsNodeHandle *addFile(std::unique_ptr<vfs2::IVfsNodeHandle> handle) {
            stdx::UniqueLock guard(lock);
            vfs2::IVfsNodeHandle *ptr = handle.get();
            files.insert(std::move(handle));
            return ptr;
        }

        vfs2::IVfsNodeHandle *findFile(const vfs2::IVfsNodeHandle *ptr) {
            // This is awful, but theres no transparent lookup for unique pointers
            std::unique_ptr<vfs2::IVfsNodeHandle> handle(const_cast<vfs2::IVfsNodeHandle*>(ptr));
            defer { (void)handle.release(); }; // NOLINT(bugprone-unused-return-value)

            stdx::SharedLock guard(lock);
            if (auto it = files.find(handle); it != files.end()) {
                return it->get();
            }

            return nullptr;
        }
    };

    struct Mutex : public KernelObject {
        stdx::SpinLock lock;

        Mutex(MutexId id, stdx::String name)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleMutex) << 56), std::move(name))
        { }

        // TODO: store a list of all the threads waiting on this mutex
    };

    class SystemObjects {
        stdx::SharedSpinLock mLock;
        sm::RcuDomain mDomain;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;

        sm::FlatHashMap<ThreadId, sm::RcuWeakPtr<Thread>> mThreads;
        sm::FlatHashMap<AddressSpaceId, sm::RcuSharedPtr<AddressSpace>> mAddressSpaces;
        sm::FlatHashMap<ProcessId, sm::RcuSharedPtr<Process>> mProcesses;
        sm::FlatHashMap<MutexId, sm::RcuSharedPtr<Mutex>> mMutexes;

    public:
        SystemObjects() = default;

        sm::RcuSharedPtr<Thread> createThread(stdx::String name, sm::RcuSharedPtr<Process> process);
        sm::RcuSharedPtr<AddressSpace> createAddressSpace(stdx::String name, km::AddressMapping mapping, km::PageFlags flags, km::MemoryType type, sm::RcuSharedPtr<Process> process);
        sm::RcuSharedPtr<Process> createProcess(stdx::String name, x64::Privilege privilege, SystemPageTables *systemTables, MemoryRange pteMemory);
        sm::RcuSharedPtr<Mutex> createMutex(stdx::String name);

        sm::RcuWeakPtr<Thread> getThread(ThreadId id);
        sm::RcuSharedPtr<AddressSpace> getAddressSpace(AddressSpaceId id);
        sm::RcuSharedPtr<Process> getProcess(ProcessId id);
        sm::RcuSharedPtr<Mutex> getMutex(MutexId id);
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
