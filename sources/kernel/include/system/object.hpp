#pragma once

#include <bezos/handle.h>

#include "std/string_view.hpp"

namespace sys {
    class IObject {
    public:
        virtual ~IObject() = default;

        virtual OsHandle publicId() const = 0;
        virtual stdx::StringView name() const = 0;

        OsHandleType type() const { return OS_HANDLE_TYPE(publicId()); }
        OsHandle privateId() const { return OS_HANDLE_ID(publicId()); }
    };
}
