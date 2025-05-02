#pragma once

#include "system/create.hpp"
#include "system/handle.hpp"

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

        BaseObject(ObjectName name) noexcept
            : mName(name)
        {
            mName.resize(strnlen(name.data(), name.capacity()));
        }

        ObjectName getNameUnlocked() const REQUIRES_SHARED(mLock) {
            return mName;
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
