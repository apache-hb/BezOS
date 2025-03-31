#pragma once

#include <bezos/facility/mutex.h>

#include "system/handle.hpp"

namespace sys2 {
    class System;
    class Mutex;
    class MutexHandle;

    enum class MutexAccess : uint64_t {
        eNone = eOsMutexAccessNone,
        eWait = eOsMutexAccessWait,
        eDestroy = eOsMutexAccessDestroy,
        eUpdate = eOsMutexAccessUpdate,
        eStat = eOsMutexAccessStat,
        eAll = eOsMutexAccessAll,
    };

    UTIL_BITFLAGS(MutexAccess);

    class MutexHandle final : public IHandle {
        sm::RcuWeakPtr<Mutex> mMutex;
        OsHandle mHandle;
        MutexAccess mAccess;

    public:
        MutexHandle(sm::RcuWeakPtr<Mutex> mutex, OsHandle handle, MutexAccess access)
            : mMutex(mutex)
            , mHandle(handle)
            , mAccess(access)
        { }

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }

        sm::RcuWeakPtr<Mutex> getMutex() const { return mMutex; }

        bool hasAccess(MutexAccess access) const {
            return bool(mAccess & access);
        }
    };

    class Mutex final : public IObject {
        stdx::SharedSpinLock mLock;
        ObjectName mName;

        sm::FlatHashSet<sm::RcuWeakPtr<Thread>> mWaiters GUARDED_BY(mLock);
        std::atomic_flag mLocked = ATOMIC_FLAG_INIT;
    public:
        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Mutex"; }
    };
}
