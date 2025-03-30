#pragma once

#include <bezos/facility/process.h>

#include "memory/address_space.hpp"
#include "memory/range.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"
#include "system/handle.hpp"
#include "system/thread.hpp"
#include "util/absl.hpp"

namespace sys2 {
    class System;
    class Thread;
    class Process;
    class ProcessHandle;

    enum class ProcessId : uint64_t {};

    enum class ProcessAccess : uint64_t {
        eNone = eOsProcessAccessNone,
        eTerminate = eOsProcessAccessTerminate,
        eSuspend = eOsProcessAccessSuspend,
        eWait = eOsProcessAccessWait,
        eVmControl = eOsProcessAccessVmControl,
        eThreadControl = eOsProcessAccessThreadControl,
        eIoControl = eOsProcessAccessIoControl,
        eProcessControl = eOsProcessAccessProcessControl,
        eQuota = eOsProcessAccessQuota,
        eAll = eOsProcessAccessAll,
    };

    UTIL_BITFLAGS(ProcessAccess);

    struct ProcessCreateInfo {
        ObjectName name;
        sm::FixedArray<std::byte> args;

        /// @brief Is this a supervisor process?
        bool supervisor;

        /// @brief Initial scheduling state.
        OsProcessStateFlags state;
    };

    struct ProcessDestroyInfo {
        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    struct ProcessInfo {
        ObjectName name;
        uint64_t handles;
        bool supervisor;
    };

    OsStatus CreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **process);

    class ProcessHandle final : public IHandle {
        /// @brief The process that this is a handle to.
        sm::RcuWeakPtr<Process> mProcess;

        /// @brief The handle identifier inside the owning process.
        OsHandle mHandle;

        /// @brief The permissions that this handle grants to the owner.
        ProcessAccess mAccess;

    public:
        ProcessHandle(sm::RcuWeakPtr<Process> process, OsHandle handle, ProcessAccess access);

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }

        sm::RcuWeakPtr<Process> getProcessObject() { return mProcess; }

        bool hasAccess(ProcessAccess access) const {
            return bool(mAccess & access);
        }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroyProcess(System *system, const ProcessDestroyInfo& info);

        OsStatus createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle);
    };

    class Process final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);
        bool mSupervisor;
        ProcessId mId;

        HandleAllocator<OsHandle> mIdAllocators[eOsHandleCount];

        sm::RcuWeakPtr<Process> mParent;

        sm::FlatHashSet<sm::RcuSharedPtr<Process>, sm::RcuHash<Process>, std::equal_to<>> mChildren;
        sm::FlatHashSet<sm::RcuSharedPtr<Thread>, sm::RcuHash<Thread>, std::equal_to<>> mThreads;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, std::unique_ptr<IHandle>> mHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector2<km::MemoryRange> mPhysicalMemory;

        km::AddressMapping mPteMemory;

        /// @brief The page tables for this process.
        km::AddressSpace mPageTables;

        void removeChild(sm::RcuSharedPtr<Process> child);
        void addChild(sm::RcuSharedPtr<Process> child);

    public:
        Process(const ProcessCreateInfo& createInfo, sm::RcuWeakPtr<Process> parent, const km::AddressSpace *systemTables, km::AddressMapping pteMemory);

        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus stat(ProcessInfo *info);
        bool isSupervisor() const { return mSupervisor; }

        IHandle *getHandle(OsHandle handle);
        void addHandle(IHandle *handle);

        OsHandle newHandleId(OsHandleType type) {
            OsHandle id = mIdAllocators[type].allocate();
            return OS_HANDLE_NEW(type, id);
        }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroy(System *system, const ProcessDestroyInfo& info);

        OsStatus createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle);

        void removeThread(sm::RcuSharedPtr<Thread> thread);
        void addThread(sm::RcuSharedPtr<Thread> thread);
    };
}
