#pragma once

#include <bezos/handle.h>

namespace sys {
    class IObject;

    class ISystem {
    public:
        virtual ~ISystem() = default;

        virtual OsStatus getObject(OsHandle handle, IObject **object) = 0;

        virtual OsHandle newHandle(OsHandleType type) = 0;
    };
}
