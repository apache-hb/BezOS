#pragma once

#include <bezos/facility/threads.h>

#include "arch/xsave.hpp"
#include "memory/layout.hpp"
#include "system/handle.hpp"
#include "system/create.hpp"
#include "xsave.hpp"

#include <stdlib.h>

namespace sys2 {
    using XSaveState = std::unique_ptr<x64::XSave, decltype(&km::DestroyXSave)>;

    struct ThreadDestroyInfo {
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

        sm::RcuSharedPtr<Thread> getThread() { return mThread; }

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }


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
