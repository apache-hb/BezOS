#include "system/thread.hpp"

#include "thread.hpp"
#include "xsave.hpp"

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
