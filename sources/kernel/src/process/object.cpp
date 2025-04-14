#include "process/system.hpp"

#include "panic.hpp"

using KernelObject = km::BaseObject;

void KernelObject::initHeader(OsHandle id, OsHandleType type, stdx::String name) {
    mId = OS_HANDLE_NEW(type, id);
    setName(std::move(name));
}

km::ObjectName km::BaseObject::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

void km::BaseObject::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
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

OsHandle km::SystemObjects::getNodeId(sm::RcuSharedPtr<vfs2::INode> node) {
    stdx::SharedLock guard(mLock);
    if (auto it = mVfsNodes.find(node); it != mVfsNodes.end()) {
        return it->second->publicId();
    }

    return OS_HANDLE_INVALID;
}

OsHandle km::SystemObjects::getHandleId(vfs2::IHandle *handle) {
    stdx::SharedLock guard(mLock);
    if (auto it = mVfsHandles.find(handle); it != mVfsHandles.end()) {
        return it->second->publicId();
    }

    return OS_HANDLE_INVALID;
}
