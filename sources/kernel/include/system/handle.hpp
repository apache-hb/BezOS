#pragma once

#include <bezos/handle.h>

#include "std/rcuptr.hpp"
#include "std/static_string.hpp"

namespace sys2 {
    class IObject;
    class IHandle;

    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    enum class HandleAccess : uint64_t {
        eNone = 0,
    };

    struct HandleCreateInfo {
        /// @brief The object that is requesting access to a new handle.
        sm::RcuSharedPtr<IObject> owner;

        /// @brief The flags that describe access to the handle.
        HandleAccess access;
    };

    class IObject : public sm::RcuIntrusivePtr<IObject> {
    public:
        virtual ~IObject() = default;

        virtual void setName(ObjectName name) = 0;
        virtual ObjectName getName() = 0;
        virtual stdx::StringView getClassName() const = 0;

        virtual OsStatus open(HandleCreateInfo, IHandle **) { return OsStatusNotSupported; }
        virtual OsStatus close(IHandle *) { return OsStatusNotSupported; }
    };

    class IHandle {
    public:
        virtual ~IHandle() = default;

        virtual sm::RcuWeakPtr<IObject> getOwner() = 0;
        virtual sm::RcuWeakPtr<IObject> getObject() = 0;
        virtual OsHandle getHandle() const = 0;

        OsHandleType getHandleType() const { return OS_HANDLE_TYPE(getHandle()); }
        OsHandle internalId() const { return OS_HANDLE_ID(getHandle()); }
    };
}
