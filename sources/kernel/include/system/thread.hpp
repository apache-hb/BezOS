#pragma once

#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "memory/layout.hpp"
#include "system/handle.hpp"

#include <stdlib.h>

namespace sys2 {
    class System;
    class Process;

    using reg_t = uint64_t;

    struct RegisterSet {
        reg_t rax;
        reg_t rbx;
        reg_t rcx;
        reg_t rdx;
        reg_t rdi;
        reg_t rsi;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
        reg_t r12;
        reg_t r13;
        reg_t r14;
        reg_t r15;
        reg_t rbp;
        reg_t rsp;
        reg_t rip;
        reg_t rflags;
    };

    using XSaveState = std::unique_ptr<x64::XSave, decltype(&free)>;

    class Thread final : public IObject {
        OsHandle mHandle;
        ObjectName mName;
        Process *mParent;

        RegisterSet mCpuState;
        XSaveState mFpuState{nullptr, &free};
        reg_t mTlsAddress;

        km::AddressMapping mUserStack;
        km::AddressMapping mKernelStack;
        OsThreadState mThreadState;

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

    struct ThreadInfo {

    };

    OsStatus CreateThread(System *system, const ThreadCreateInfo& info, Thread **thread);
    OsStatus DestroyThread(System *system, const ThreadDestroyInfo& info, Thread *thread);
    OsStatus StatThread(System *system, Thread *thread, OsThreadInfo *info);
}
