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

        sm::RcuWeakPtr<IObject> getObject() override { return mObject; }
        OsHandle getHandle() const override { return mHandle; }

        sm::RcuSharedPtr<T> getInner() const { return mObject; }

        bool hasAccess(Access access) const {
            return bool(mAccess & access);
        }
    };

    class BaseObject : public IObject {
        ObjectName mName GUARDED_BY(mLock);

    protected:
        stdx::SharedSpinLock mLock;

    public:
        void setName(ObjectName name) override {
            stdx::UniqueLock guard(mLock);
            mName = name;
        }

        ObjectName getName() override {
            stdx::SharedLock guard(mLock);
            return mName;
        }
    };
}
