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

        /// @brief Fault in a range of the backing object
        ///
        /// @param page the page offset that has been faulted
        /// @param[out] result The resulting physical memory to map
        ///
        /// @return The status of the fault operation
        virtual OsStatus fault(size_t page, km::PhysicalAddress *address) = 0;

        /// @brief Signal that a page has been unmapped
        virtual OsStatus release(size_t page) = 0;

        virtual km::MemoryRange range() = 0;
    };
}
