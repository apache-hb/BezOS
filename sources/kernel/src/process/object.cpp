#include "process/process.hpp"

using KernelObject = km::KernelObject;

void KernelObject::initHeader(OsHandle id, OsHandleType type, stdx::String name) {
    mId = OS_HANDLE_NEW(type, id);
    mName = std::move(name);
}

bool KernelObject::retainStrong() {
    if (mStrongCount == 0) {
        return false;
    }

    mStrongCount += 1;
    retainWeak();

    return true;
}

void KernelObject::retainWeak() {
    mWeakCount += 1;
}

bool KernelObject::releaseStrong() {
    KM_CHECK(mStrongCount > 0, "Invalid strong count.");

    mStrongCount -= 1;

    return releaseWeak();
}

bool KernelObject::releaseWeak() {
    KM_CHECK(mWeakCount > 0, "Invalid weak count.");

    mWeakCount -= 1;
    if (mWeakCount == 0) {
        return true;
    }

    return false;
}
