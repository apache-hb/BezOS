#pragma once

#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "memory/layout.hpp"
#include "system/base.hpp"
#include "system/create.hpp"
#include "xsave.hpp"

#include <stdlib.h>

namespace sys2 {
    using XSaveState = std::unique_ptr<x64::XSave, decltype(&km::DestroyXSave)>;

    XSaveState NewXSaveState() [[clang::allocating]];

    class Thread final : public BaseObject {
        sm::RcuWeakPtr<Process> mProcess;

        RegisterSet mCpuState;
        XSaveState mFpuState{nullptr, &km::DestroyXSave};
        reg_t mTlsAddress;

        [[maybe_unused]]
        km::StackMapping mKernelStack;

        [[maybe_unused]]
        OsThreadState mThreadState;

    public:
        using Access = ThreadAccess;
        using Super = BaseObject;

        Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack);
        Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, sys2::XSaveState fpuState, km::StackMapping kernelStack);

        stdx::StringView getClassName() const override { return "Thread"; }

        OsStatus stat(ThreadStat *info);

        void saveState(RegisterSet& regs);
        RegisterSet loadState();
        reg_t getTlsAddress() const { return mTlsAddress; }
        km::StackMapping getKernelStack() const { return mKernelStack; }

        bool isSupervisor();

        OsStatus destroy(System *system, const ThreadDestroyInfo& info);
    };

    class ThreadHandle final : public BaseHandle<Thread> {
    public:
        using Super = BaseHandle<Thread>;

        ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access);

        sm::RcuSharedPtr<Thread> getThread() { return getInner(); }

        OsStatus destroy(System *system, const ThreadDestroyInfo& info);
    };
}
