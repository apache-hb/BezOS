#pragma once

#include <bezos/handle.h>

#include "std/rcuptr.hpp"
#include "system/create.hpp"

namespace sys2 {
    struct HandleCreateInfo {
        /// @brief The process that is requesting access to a new handle.
        sm::RcuSharedPtr<Process> owner;

        /// @brief The flags that describe access to the handle.
        OsHandleAccess access;
    };

    template<typename T>
    class HandleAllocator {
        std::atomic<T> mCurrent = OS_HANDLE_INVALID + 1;
    public:
        T allocate() {
            return T(mCurrent.fetch_add(1));
        }
    };

    class IObject : public sm::RcuIntrusivePtr<IObject> {
    public:
        virtual ~IObject() = default;

        virtual void setName(ObjectName name) = 0;
        virtual ObjectName getName() = 0;
        virtual stdx::StringView getClassName() const = 0;
        virtual OsHandleType getObjectType() const = 0;

        virtual OsStatus open(HandleCreateInfo, IHandle **) { return OsStatusNotSupported; }
        virtual OsStatus close(IHandle *) { return OsStatusNotSupported; }
    };

    class IHandle {
    public:
        virtual ~IHandle() = default;

        virtual sm::RcuWeakPtr<IObject> getObject() = 0;
        virtual OsHandle getHandle() const = 0;
        virtual OsHandleAccess getAccess() const = 0;
        virtual OsStatus clone(OsHandleAccess, OsHandle, IHandle **) { return OsStatusNotSupported; }
        virtual OsStatus close() { return OsStatusNotSupported; }

        OsHandleType getHandleType() const { return OS_HANDLE_TYPE(getHandle()); }
        OsHandle internalId() const { return OS_HANDLE_ID(getHandle()); }
        bool hasGenericAccess(OsHandleAccess access) const {
            return (getAccess() & access) == access;
        }
    };
}
