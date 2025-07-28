#pragma once

#include <bezos/facility/thread.h>

#include "arch/xsave.hpp"
#include "memory/layout.hpp"
#include "system/base.hpp"
#include "system/create.hpp"
#include "task/scheduler_queue.hpp"
#include "xsave.hpp"

#include <stdlib.h>

namespace task {
    class Scheduler;
}

namespace sys {
    using XSaveState = std::unique_ptr<x64::XSave, decltype(&km::DestroyXSave)>;

    XSaveState NewXSaveState() [[clang::allocating]];

    class ThreadSchedulerEntry : public task::SchedulerEntry {
    protected:
        sm::RcuWeakPtr<Process> mProcess;
        km::StackMapping mKernelStack;

    public:
        sm::RcuSharedPtr<Process> getProcess() {
            return mProcess.lock();
        }
    };

    class Thread final
        : public BaseObject<eOsHandleThread>
        , public ThreadSchedulerEntry
    {
        RegisterSet mCpuState;
        XSaveState mFpuState{nullptr, &km::DestroyXSave};
        reg_t mTlsAddress;
        std::atomic<OsThreadState> mThreadState;

    public:
        using Access = ThreadAccess;
        using Super = BaseObject;

        Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack);
        Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, sys::XSaveState fpuState, km::StackMapping kernelStack);
        Thread(OsThreadCreateInfo createInfo, sm::RcuWeakPtr<Process> process, sys::XSaveState fpuState, km::StackMapping kernelStack);

        stdx::StringView getClassName() const override { return "Thread"; }

        OsStatus stat(ThreadStat *info);

        void saveState(RegisterSet& regs);
        RegisterSet loadState();
        reg_t getTlsAddress() const { return mTlsAddress; }
        km::StackMapping getKernelStack() const { return mKernelStack; }

        bool isSupervisor();

        void setSignalStatus(OsStatus status);

        OsStatus suspend();
        OsStatus resume();

        OsStatus attach(task::Scheduler *scheduler);

        km::PhysicalAddress getPageMap();

        OsStatus destroy(System *system, OsThreadState reason);
        OsThreadState state() const { return mThreadState.load(); }

        bool cmpxchgState(OsThreadState &expected, OsThreadState newState) {
            return mThreadState.compare_exchange_strong(expected, newState);
        }
    };

    class ThreadHandle final : public BaseHandle<Thread> {
    public:
        ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access);

        sm::RcuSharedPtr<Thread> getThread() { return getInner(); }
    };
}
