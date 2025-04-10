#pragma once

#include "std/rcuptr.hpp"
#include "system/access.hpp"

namespace sys2 {
    /// @brief An isolation context for a transaction
    class IIsolate : public sm::RcuIntrusivePtr<IIsolate> {
    public:
        virtual ~IIsolate() = default;

        virtual OsStatus addObject(sm::RcuSharedPtr<IObject> object) = 0;
        virtual OsStatus commit() = 0;
        virtual OsStatus rollback() = 0;
    };
}
