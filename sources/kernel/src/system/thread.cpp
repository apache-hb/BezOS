#include "system/thread.hpp"
#include "system/process.hpp"

#include "thread.hpp"
#include "xsave.hpp"

void sys2::Thread::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
    mName = name;
}

sys2::ObjectName sys2::Thread::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

OsStatus sys2::Thread::open(OsHandle*) {
    return OsStatusNotSupported;
}

OsStatus sys2::Thread::close(OsHandle) {
    return OsStatusNotSupported;
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
