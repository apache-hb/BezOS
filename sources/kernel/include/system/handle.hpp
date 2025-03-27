#pragma once

#include <bezos/handle.h>

#include "std/rcuptr.hpp"
#include "std/static_string.hpp"

namespace sys2 {
    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    class IObject : public sm::RcuIntrusivePtr<IObject> {
    public:
        virtual ~IObject() = default;

        virtual void setName(ObjectName name) = 0;
        virtual ObjectName getName() const = 0;
        virtual stdx::StringView getClassName() const = 0;

        virtual OsStatus open(OsHandle *handle) = 0;
        virtual OsStatus close(OsHandle handle) = 0;
    };

    class Handle {
        sm::RcuWeakPtr<IObject> mObject;
        [[maybe_unused]]
        OsHandle mHandle;
    public:
    };
}
