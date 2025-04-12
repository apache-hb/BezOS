#pragma once

#include "system/base.hpp"
#include "system/create.hpp"

namespace sys2 {
    class Mutex final : public BaseObject<eOsHandleMutex> {
        sm::RcuWeakPtr<Thread> mOwner;
        sm::FlatHashSet<sm::RcuWeakPtr<Thread>> mWaiters GUARDED_BY(mLock);
    public:
        using Access = MutexAccess;

        stdx::StringView getClassName() const override { return "Mutex"; }

        bool tryAcquireLock(sm::RcuWeakPtr<Thread> thread);
    };

    class MutexHandle final : public BaseHandle<Mutex, eOsHandleMutex> {
    public:
        MutexHandle(sm::RcuSharedPtr<Mutex> mutex, OsHandle handle, MutexAccess access)
            : BaseHandle(mutex, handle, access)
        { }

        sm::RcuWeakPtr<Mutex> getMutex() const { return getInner(); }
    };
}
