#pragma once

#include "system/create.hpp"
#include "system/handle.hpp"

namespace sys2 {
    template<typename T>
    class BaseHandle : public IHandle {
        using Access = typename T::Access;

        sm::RcuSharedPtr<T> mObject;
        OsHandle mHandle;
        Access mAccess;

    public:
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
            return bool(mAccess & access);
        }
    };

    class BaseObject : public IObject {
        ObjectName mName GUARDED_BY(mLock);

    protected:
        stdx::SharedSpinLock mLock;

        BaseObject(ObjectName name) noexcept
            : mName(name)
        { }

        ObjectName getNameUnlocked() const REQUIRES_SHARED(mLock) {
            return mName;
        }

    public:
        void setName(ObjectName name) override final {
            stdx::UniqueLock guard(mLock);
            mName = name;
        }

        ObjectName getName() override final {
            stdx::SharedLock guard(mLock);
            return getNameUnlocked();
        }

        stdx::SharedSpinLock& getMonitor() { return mLock; }
    };
}
