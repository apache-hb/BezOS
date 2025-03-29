#pragma once

#include <bezos/facility/process.h>

#include "memory/address_space.hpp"
#include "memory/range.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"
#include "system/handle.hpp"
#include "util/absl.hpp"

namespace sys2 {
    class System;
    class Thread;
    class Process;
    class ProcessHandle;

    enum class ProcessId : uint64_t {};
    enum class ProcessAccess : uint64_t {};

    struct ProcessCreateInfo {
        sm::RcuWeakPtr<Process> parent;
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

    OsStatus CreateProcess(System *system, ProcessCreateInfo info, sm::RcuSharedPtr<Process> *process);
    OsStatus DestroyProcess(System *system, const ProcessDestroyInfo& info, sm::RcuWeakPtr<Process> process);

    class ProcessHandle final : public IHandle {
        sm::RcuWeakPtr<Process> mProcess;
        sm::RcuWeakPtr<Process> mOwner;
        OsHandle mHandle;
        ProcessAccess mAccess;

    public:
        ProcessHandle(sm::RcuWeakPtr<Process> process, OsHandle handle, ProcessAccess access);

        sm::RcuWeakPtr<IObject> getOwner() override;
        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }
    };

    class Process final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);
        bool mSupervisor;
        ProcessId mId;

        sm::RcuWeakPtr<Process> mParent;

        sm::FlatHashSet<sm::RcuWeakPtr<Thread>> mThreads;
        sm::FlatHashSet<sm::RcuWeakPtr<Process>> mChildren;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, std::unique_ptr<IHandle>> mHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector2<km::MemoryRange> mPhysicalMemory;

        km::AddressMapping mPteMemory;

        /// @brief The page tables for this process.
        km::AddressSpace mPageTables;

        void addChild(sm::RcuWeakPtr<Process> child);
        void removeChild(sm::RcuWeakPtr<Process> child);

        friend OsStatus sys2::CreateProcess(System *system, ProcessCreateInfo info, sm::RcuSharedPtr<Process> *process);
        friend OsStatus sys2::DestroyProcess(System *system, const ProcessDestroyInfo& info, sm::RcuWeakPtr<Process> process);

    public:
        Process(const ProcessCreateInfo& createInfo, const km::AddressSpace *systemTables, km::AddressMapping pteMemory);

        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus stat(ProcessInfo *info);
        bool isSupervisor() const { return mSupervisor; }

#if __STDC_HOSTED__
        bool TESTING_hasChildProcess(sm::RcuWeakPtr<Process> child) {
            stdx::SharedLock guard(mLock);
            return mChildren.contains(child);
        }
#endif
    };
}
