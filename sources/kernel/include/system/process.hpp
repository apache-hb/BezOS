#pragma once

#include <bezos/handle.h>
#include <bezos/facility/process.h>

#include "memory/range.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"
#include "system/handle.hpp"

namespace sys2 {
    class System;

    class Process final : public IObject {
        stdx::SharedSpinLock mLock;
        OsHandle mHandle;
        ObjectName mName;

        /// @brief All the handles this process has open.
        stdx::Vector3<OsHandle> mHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector3<km::MemoryRange> mPhysicalMemory;

    public:
        void setName(ObjectName name) override;
        ObjectName getName() const override;

        OsHandle handle() const override;
    };

    struct ProcessCreateInfo {
        Process *parent;
        ObjectName name;
    };

    struct ProcessDestroyInfo {
        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    OsStatus CreateProcess(System *system, const ProcessCreateInfo& info, Process **process);
    OsStatus DestroyProcess(System *system, const ProcessDestroyInfo& info, Process *process);
    OsStatus StatProcess(System *system, Process *process, OsProcessInfo *info);
}
