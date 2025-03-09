#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "memory/tables.hpp"
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
    enum class VNodeId : OsDeviceHandle { };

    template<typename T>
    class IdAllocator {
        std::atomic<std::underlying_type_t<T>> mCurrent = OS_HANDLE_INVALID + 1;

    public:
        T allocate() {
            return T(mCurrent.fetch_add(1));
        }
    };

    class KernelObject;
    struct Thread;
    struct AddressSpace;
    struct Process;
    struct Mutex;

    class SystemObjects;

    struct HandleWait {
        KernelObject *object = nullptr;
        OsInstant timeout = OS_TIMEOUT_INSTANT;
    };

    class KernelObject {
        OsHandle mId;
        stdx::String mName;
        stdx::SpinLock mMonitor;

        uint32_t mStrongCount = 1;
        uint32_t mWeakCount = 1;

    public:
        virtual ~KernelObject() = default;

        KernelObject() = default;

        KernelObject(OsHandle id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
        { }

        void initHeader(OsHandle id, OsHandleType type, stdx::String name);

        OsHandle publicId() const { return mId; }
        OsHandleType handleType() const { return OS_HANDLE_TYPE(mId); }
        OsHandle internalId() const { return OS_HANDLE_ID(mId); }

        stdx::StringView name() const { return mName; }

        bool retainStrong();
        void retainWeak();

        bool releaseStrong();
        bool releaseWeak();
    };

    struct Thread : public KernelObject {
        Thread() = default;

        Thread(ThreadId id, stdx::String name, Process *process)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleThread) << 56), std::move(name))
            , process(process)
        { }

        void init(ThreadId id, stdx::String name, Process *process, AddressMapping kernelStack);

        Process *process;
        HandleWait wait;
        km::IsrContext state;
        uint64_t tlsAddress = 0;
        AddressSpace *stack;
        AddressMapping kernelStack;
        OsThreadState threadState = eOsThreadRunning;

        OsStatus waitOnHandle(KernelObject *object, OsInstant timeout);
        bool isComplete() const {
            return threadState != eOsThreadFinished;
        }

        void *getSyscallStack() const {
            return (void*)((uintptr_t)kernelStack.vaddr + kernelStack.size - x64::kPageSize);
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

    struct Process : public KernelObject {
        Process() = default;

        x64::Privilege privilege;
        stdx::SharedSpinLock lock;
        ProcessPageTables ptes;
        stdx::Vector2<AddressSpace*> memory;
        sm::FlatHashSet<std::unique_ptr<vfs2::IVfsNodeHandle>> files;
        sm::FlatHashMap<OsHandle, KernelObject*> handles;
        OsProcessState state = { eOsProcessRunning };

        void init(ProcessId id, stdx::String name, x64::Privilege protection, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea);

        bool isComplete() const;

        void addAddressSpace(AddressSpace *addressSpace);

        vfs2::IVfsNodeHandle *addFile(std::unique_ptr<vfs2::IVfsNodeHandle> handle);
        OsStatus closeFile(vfs2::IVfsNodeHandle *ptr);
        vfs2::IVfsNodeHandle *findFile(const vfs2::IVfsNodeHandle *ptr);

        void addHandle(KernelObject *object);
        KernelObject *findHandle(OsHandle id);
        OsStatus removeHandle(OsHandle id);
    };

    struct Mutex : public KernelObject {
        stdx::SpinLock lock;

        Mutex(MutexId id, stdx::String name)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleMutex) << 56), std::move(name))
        { }

        // TODO: store a list of all the threads waiting on this mutex
    };

    struct VNode : public KernelObject {
        std::unique_ptr<vfs2::IVfsNodeHandle> node;

        void init(VNodeId id, stdx::String name, std::unique_ptr<vfs2::IVfsNodeHandle> node);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };
}
