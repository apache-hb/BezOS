#pragma once

#include <bezos/handle.h>
#include <bezos/facility/process.h>

#include "std/shared_spinlock.hpp"
#include "system/handle.hpp"

namespace sys2 {
    class System;

    class Process : public IObject {
        stdx::SharedSpinLock mLock;
        OsHandle mHandle;
        ObjectName mName;

    public:
        void setName(ObjectName name) override;
        ObjectName getName() const override;

        OsHandle handle() const override;
    };

    struct ProcessCreateInfo {
        Process *parent;
        stdx::StaticString<32> name;
    };

    struct ProcessDestroyInfo {
        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    OsStatus CreateProcess(System *system, const ProcessCreateInfo &info, Process **process);
    OsStatus DestroyProcess(System *system, Process *process);
}
