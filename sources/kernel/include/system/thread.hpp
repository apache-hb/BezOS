#pragma once

#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "system/handle.hpp"

#include <stdlib.h>

namespace sys2 {
    class System;
    class Process;

    class Thread final : public IObject {
        OsHandle mHandle;
        ObjectName mName;

        std::unique_ptr<x64::XSave, decltype(&free)> mFpuState{nullptr, &free};

    public:
        void setName(ObjectName name) override;
        ObjectName getName() const override;

        OsHandle handle() const override;
    };

    struct ThreadCreateInfo {
        Process *parent;
        ObjectName name;
        OsThreadEntry entry;
        OsAnyPointer argument;
        OsSize userStackSize;
        OsSize kernelStackSize;
    };

    struct ThreadDestroyInfo {
        int64_t exitCode;
        OsThreadState reason;
    };

    OsStatus CreateThread(System *system, const ThreadCreateInfo& info, Thread **thread);
    OsStatus DestroyThread(System *system, const ThreadDestroyInfo& info, Thread *thread);
    OsStatus StatThread(System *system, Thread *thread, OsThreadInfo *info);
}
