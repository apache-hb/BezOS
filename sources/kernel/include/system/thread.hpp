#pragma once

#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "memory/layout.hpp"
#include "system/handle.hpp"
#include "xsave.hpp"

#include <stdlib.h>

namespace sys2 {
    class System;
    class Process;
    class ProcessHandle;
    class Thread;
    class ThreadHandle;

    enum class ThreadAccess : OsHandleAccess {
        eNone = eOsThreadAccessNone,
        eWait = eOsThreadAccessWait,
        eTerminate = eOsThreadAccessTerminate,
        eStat = eOsThreadAccessStat,
        eSuspend = eOsThreadAccessSuspend,
        eQuota = eOsThreadAccessQuota,
        eAll = eOsThreadAccessAll,
    };

    UTIL_BITFLAGS(ThreadAccess);

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

    using XSaveState = std::unique_ptr<x64::XSave, decltype(&km::DestroyXSave)>;

    struct ThreadCreateInfo {
        ObjectName name;

        /// @brief The initial cpu state for the thread.
        /// @note Only the syscall stack is allocated by the kernel, all other state is managed
        ///       by the userspace program. This puts the onus on the program to ensure that
        ///       exiting a thread will clean up all the state it has allocated aside from the thread
        ///       object itself, which is managed by the kernel.
        RegisterSet cpuState;
        reg_t tlsAddress;
        OsThreadState state;

        OsSize kernelStackSize;
    };

    struct ThreadDestroyInfo {
        int64_t exitCode;
        OsThreadState reason;
    };

    struct ThreadInfo {
        ObjectName name;
        OsThreadState state;
        sm::RcuWeakPtr<Process> process;
    };

    class ThreadHandle final : public IHandle {
        sm::RcuSharedPtr<Thread> mThread;
        OsHandle mHandle;
        ThreadAccess mAccess;

    public:
        ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access);

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }

        sm::RcuSharedPtr<Thread> getThreadObject() { return mThread; }

        bool hasAccess(ThreadAccess access) const {
            return bool(mAccess & access);
        }

        OsStatus destroy(System *system, const ThreadDestroyInfo& info);
    };

    class Thread final : public IObject {
        stdx::SharedSpinLock mLock;

        ObjectName mName GUARDED_BY(mLock);

        sm::RcuWeakPtr<Process> mProcess;

        RegisterSet mCpuState;
        XSaveState mFpuState{nullptr, &km::DestroyXSave};
        reg_t mTlsAddress;

        [[maybe_unused]]
        km::StackMapping mKernelStack;

        [[maybe_unused]]
        OsThreadState mThreadState;

    public:
        Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack);

        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Thread"; }

        OsStatus stat(ThreadInfo *info);

        void saveState(RegisterSet& regs);
        RegisterSet loadState();
        reg_t getTlsAddress() const { return mTlsAddress; }
        km::StackMapping getKernelStack() const { return mKernelStack; }

        bool isSupervisor();

        OsStatus destroy(System *system, const ThreadDestroyInfo& info);
    };
}
