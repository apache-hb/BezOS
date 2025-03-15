#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "fs2/base.hpp"
#include "memory/tables.hpp"
#include "isr/isr.hpp"
#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"
#include "std/string.hpp"

#include "util/absl.hpp"

namespace km {
    class SystemMemory;

    enum class ThreadId : OsThreadHandle { };
    enum class ProcessId : OsProcessHandle { };
    enum class MutexId : OsMutexHandle { };
    enum class NodeId : OsNodeHandle { };
    enum class DeviceId : OsDeviceHandle { };

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

        /// @brief Pointer to the xsave state for this thread
        //
        // This points to a block of memory that will be larger than sizeof(x64::XSave) as the xsave
        // area changes size based on enabled features and processor feature support.
        std::unique_ptr<x64::XSave, decltype(&free)> xsave{nullptr, &free};

        uint64_t tlsAddress = 0;
        AddressMapping userStack;
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

    struct Process : public KernelObject {
        Process() = default;

        x64::Privilege privilege;
        stdx::SharedSpinLock lock;
        ProcessPageTables ptes;
        sm::FlatHashMap<OsHandle, KernelObject*> handles;
        OsProcessInfo state = { eOsProcessRunning };

        void init(ProcessId id, stdx::String name, x64::Privilege protection, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea);

        bool isComplete() const;

        void addHandle(KernelObject *object);
        KernelObject *findHandle(OsHandle id);
        OsStatus removeHandle(OsHandle id);
    };

    struct Mutex : public KernelObject {
        stdx::SpinLock lock;

        Mutex(MutexId id, stdx::String name)
            : KernelObject(std::to_underlying(id) | (uint64_t(eOsHandleMutex) << 56), std::move(name))
        { }
    };

    struct Node : public KernelObject {
        vfs2::INode *node = nullptr;

        void init(NodeId id, stdx::String name, vfs2::INode *vfsNode);
    };

    struct Device : public KernelObject {
        std::unique_ptr<vfs2::IHandle> handle;

        void init(DeviceId id, stdx::String name, std::unique_ptr<vfs2::IHandle> vfsHandle);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };
}
