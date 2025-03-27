#pragma once

#include <bezos/facility/process.h>

#include "memory/range.hpp"
#include "memory/tables.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"
#include "system/handle.hpp"
#include "util/absl.hpp"

namespace sys2 {
    class System;
    class Thread;
    class Process;

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

    class Process final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);
        bool mSupervisor;

        sm::FlatHashSet<sm::RcuSharedPtr<Thread>> mThreads;
        sm::FlatHashSet<sm::RcuSharedPtr<Process>> mChildren;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, sm::RcuSharedPtr<IObject>> mHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector3<km::MemoryRange> mPhysicalMemory;

        /// @brief The page tables for this process.
        km::ProcessPageTables mPageTables;

    public:
        Process(const ProcessCreateInfo& createInfo);

        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus open(OsHandle *handle) override;
        OsStatus close(OsHandle handle) override;

        OsStatus stat(ProcessInfo *info);
        bool isSupervisor() const { return mSupervisor; }
    };

    OsStatus CreateProcess(System *system, const ProcessCreateInfo& info, sm::RcuSharedPtr<Process> *process);
    OsStatus DestroyProcess(System *system, const ProcessDestroyInfo& info, sm::RcuSharedPtr<Process> process);
}
