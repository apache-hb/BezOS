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

    class Process final : public IObject {
        stdx::SharedSpinLock mLock;
        ObjectName mName;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, IObject*> mHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector3<km::MemoryRange> mPhysicalMemory;

        /// @brief The page tables for this process.
        km::ProcessPageTables mPageTables;

    public:
        void setName(ObjectName name) override;
        ObjectName getName() const override;
    };

    struct ProcessCreateInfo {
        Process *parent;
        ObjectName name;
    };

    struct ProcessDestroyInfo {
        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    struct ProcessInfo {

    };

    OsStatus CreateProcess(System *system, const ProcessCreateInfo& info, Process **process);
    OsStatus DestroyProcess(System *system, const ProcessDestroyInfo& info, Process *process);
    OsStatus StatProcess(System *system, Process *process, OsProcessInfo *info);
}
