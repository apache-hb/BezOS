#pragma once

#include <bezos/handle.h>

#include "std/static_string.hpp"

namespace sys2 {
    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    class IObject {
    public:
        virtual ~IObject() = default;

        virtual void setName(ObjectName name) = 0;
        virtual ObjectName getName() const = 0;

        virtual OsHandle handle() const = 0;

        OsHandleType type() const { return OS_HANDLE_TYPE(handle()); }
        OsHandle publicId() const { return handle(); }
        OsHandle internalId() const { return OS_HANDLE_ID(handle()); }
    };
}
