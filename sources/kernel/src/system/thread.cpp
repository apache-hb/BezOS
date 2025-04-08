#include "system/thread.hpp"
#include "system/process.hpp"

#include "system/system.hpp"
#include "thread.hpp"
#include "xsave.hpp"

sys2::ThreadHandle::ThreadHandle(sm::RcuSharedPtr<Thread> thread, OsHandle handle, ThreadAccess access)
    : mThread(thread)
    , mHandle(handle)
    , mAccess(access)
{ }

sm::RcuWeakPtr<sys2::IObject> sys2::ThreadHandle::getObject() {
    return mThread;
}

OsStatus sys2::ThreadHandle::destroy(System *system, const ThreadDestroyInfo& info) {
    if (!hasAccess(ThreadAccess::eTerminate)) {
        return OsStatusAccessDenied;
    }

    return mThread->destroy(system, info);
}

void sys2::Thread::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
    mName = name;
}

sys2::ObjectName sys2::Thread::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

OsStatus sys2::Thread::stat(ThreadInfo *info) {
    stdx::SharedLock guard(mLock);

    *info = ThreadInfo {
        .name = mName,
        .state = mThreadState,
        .process = mProcess,
    };

    return OsStatusSuccess;
}

void sys2::Thread::saveState(RegisterSet& regs) {
    mCpuState = regs;
    mTlsAddress = IA32_FS_BASE.load();
    if (mFpuState) {
        km::XSaveStoreState(mFpuState.get());
    }
}

sys2::RegisterSet sys2::Thread::loadState() {
    IA32_FS_BASE = mTlsAddress;
    if (mFpuState) {
        km::XSaveLoadState(mFpuState.get());
    }

    return mCpuState;
}

bool sys2::Thread::isSupervisor() {
    if (auto parent = mProcess.lock()) {
        return parent->isSupervisor();
    }

    return false;
}

sys2::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, x64::XSave *fpuState, km::StackMapping kernelStack)
    : Thread(createInfo, process, sys2::XSaveState{fpuState, &km::DestroyXSave}, kernelStack)
{ }

sys2::Thread::Thread(const ThreadCreateInfo& createInfo, sm::RcuWeakPtr<Process> process, sys2::XSaveState fpuState, km::StackMapping kernelStack)
    : mProcess(process)
    , mCpuState(createInfo.cpuState)
    , mFpuState(std::move(fpuState))
    , mTlsAddress(createInfo.tlsAddress)
    , mKernelStack(kernelStack)
    , mThreadState(createInfo.state)
{ }

OsStatus sys2::Thread::destroy(System *system, const ThreadDestroyInfo& info) {
    if (OsStatus status = system->releaseStack(mKernelStack)) {
        return status;
    }

    if (auto process = mProcess.lock()) {
        process->removeThread(loanShared());
    }

    mThreadState = info.reason;

    system->removeObject(loanWeak());
    return OsStatusSuccess;
}

sys2::XSaveState sys2::NewXSaveState() {
    return sys2::XSaveState { km::CreateXSave(), &km::DestroyXSave };
}
