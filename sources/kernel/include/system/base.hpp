#pragma once

#include "std/shared_spinlock.hpp"
#include "system/create.hpp"
#include "system/handle.hpp"

#include "std/mutex.h"

namespace sys {
    template<typename T>
    class BaseHandle : public IHandle {
        using Access = typename T::Access;

        sm::RcuSharedPtr<T> mObject;
        OsHandle mHandle;
        Access mAccess;

    public:
        static constexpr auto kHandleType = T::kHandleType;

        BaseHandle(sm::RcuSharedPtr<T> object, OsHandle handle, Access access)
            : mObject(object)
            , mHandle(handle)
            , mAccess(access)
        { }

        sm::RcuWeakPtr<IObject> getObject() override final { return mObject; }
        OsHandle getHandle() const override final { return mHandle; }
        OsHandleAccess getAccess() const override final { return OsHandleAccess(mAccess); }

        sm::RcuSharedPtr<T> getInner() const { return mObject; }

        bool hasAccess(Access access) const {
            return (mAccess & access) == access;
        }
    };

    template<OsHandleType HandleType>
    class BaseObject : public IObject {
        ObjectName mName GUARDED_BY(mLock);

    protected:
        stdx::SharedSpinLock mLock;

        BaseObject(ObjectName name) {
            setNameUnlocked(name);
        }

        ObjectName getNameUnlocked() const REQUIRES_SHARED(mLock) {
            return mName;
        }

        void setNameUnlocked(ObjectName name) REQUIRES(mLock) {
            mName = name;
            mName.resize(strnlen(name.data(), name.capacity()));
        }

    public:
        static constexpr OsHandleType kHandleType = HandleType;

        void setName(ObjectName name) override final {
            stdx::UniqueLock guard(mLock);
            mName = name;
        }

        ObjectName getName() override final {
            stdx::SharedLock guard(mLock);
            return getNameUnlocked();
        }

        OsHandleType getObjectType() const override final {
            return HandleType;
        }

        stdx::SharedSpinLock& getMonitor() { return mLock; }
    };
}
