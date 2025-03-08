#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "memory/tables.hpp"
#include "util/defer.hpp"
#include "fs2/node.hpp"
#include "isr/isr.hpp"
#include "memory/memory.hpp"
#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"
#include "std/string.hpp"

#include "util/absl.hpp"
#include "std/vector.hpp"

namespace km {
    class SystemMemory;

    enum class ThreadId : OsThreadHandle { };
    enum class AddressSpaceId : OsAddressSpaceHandle { };
    enum class ProcessId : OsProcessHandle { };
    enum class MutexId : OsMutexHandle { };

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

    class KernelObject {
        OsHandle mId;
        stdx::String mName;

    protected:
        KernelObject(OsHandle id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
        { }

    public:
        virtual ~KernelObject() = default;

        OsHandle handleId() const { return mId; }
        OsHandle internalId() const { return mId & 0x00FF'FFFF'FFFF'FFFF; }
        stdx::StringView name() const { return mName; }
    };

    struct Thread : public KernelObject {
        Thread(ThreadId id, stdx::String name, Process *process)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleThread) << 56), std::move(name))
            , process(process)
        { }

        Process *process;
        Mutex *wait;
        km::IsrContext state;
        uint64_t tlsAddress = 0;
        AddressSpace *stack;
        AddressMapping syscallStack;

        void *getSyscallStack() const {
            return (void*)((uintptr_t)syscallStack.vaddr + syscallStack.size - x64::kPageSize);
        }
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

    enum class ProcessStatus {
        eSuspended,
        eRunning,
        eExited,
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
        stdx::Vector2<Thread*> threads;
        stdx::Vector2<AddressSpace*> memory;
        sm::FlatHashSet<std::unique_ptr<vfs2::IVfsNodeHandle>> files;
        int64_t exitStatus = 0;
        ProcessStatus status = ProcessStatus::eRunning;

        void addThread(Thread *thread) {
            stdx::UniqueLock guard(lock);
            threads.add(thread);
        }

        void addAddressSpace(AddressSpace *addressSpace) {
            stdx::UniqueLock guard(lock);
            memory.add(addressSpace);
        }

        vfs2::IVfsNodeHandle *addFile(std::unique_ptr<vfs2::IVfsNodeHandle> handle) {
            stdx::UniqueLock guard(lock);
            vfs2::IVfsNodeHandle *ptr = handle.get();
            files.insert(std::move(handle));
            return ptr;
        }

        OsStatus closeFile(vfs2::IVfsNodeHandle *ptr) {
            stdx::UniqueLock guard(lock);
            // This is awful, but theres no transparent lookup for unique pointers
            std::unique_ptr<vfs2::IVfsNodeHandle> handle(const_cast<vfs2::IVfsNodeHandle*>(ptr));
            defer { (void)handle.release(); }; // NOLINT(bugprone-unused-return-value)

            if (auto it = files.find(handle); it != files.end()) {
                files.erase(it);
                return OsStatusSuccess;
            }

            return OsStatusNotFound;
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
        SystemMemory *mMemory = nullptr;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;
        IdAllocator<MutexId> mMutexIds;

        sm::FlatHashMap<ThreadId, Thread*> mThreads;
        sm::FlatHashMap<AddressSpaceId, AddressSpace*> mAddressSpaces;
        sm::FlatHashMap<ProcessId, Process*> mProcesses;
        sm::FlatHashMap<MutexId, Mutex*> mMutexes;

    public:
        SystemObjects(SystemMemory *memory)
            : mMemory(memory)
        { }

        OsStatus createProcess(stdx::String name, x64::Privilege privilege, MemoryRange pteMemory, Process **process);
        OsStatus destroyProcess(Process *process);
        Process *getProcess(ProcessId id);
        OsStatus exitProcess(Process *process, int64_t status);

        Thread *createThread(stdx::String name, Process* process);
        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping, km::PageFlags flags, km::MemoryType type, Process* process);
        Process *createProcess(stdx::String name, x64::Privilege privilege, MemoryRange pteMemory);
        Mutex *createMutex(stdx::String name);

        Thread *getThread(ThreadId id);
        AddressSpace *getAddressSpace(AddressSpaceId id);
        Mutex *getMutex(MutexId id);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };
}
