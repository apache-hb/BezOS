#pragma once

#include <bezos/status.h>

#include "std/rcuptr.hpp"

namespace sys2 {
    /// @brief Physical storage for vm objects
    ///
    /// Can represent any backing storage
    class IMemoryObject : public sm::RcuIntrusivePtr<IMemoryObject> {
    public:
        virtual ~IMemoryObject() = default;

        /// @brief Signal that a page has been unmapped
        virtual OsStatus release(size_t page) = 0;

        virtual km::MemoryRange range() = 0;
    };
}
