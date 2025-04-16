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
        /// @param range The range to fault in
        /// @param base The base address of the range
        /// @param[out] result The resulting physical memory to map
        ///
        /// @return The status of the fault operation
        virtual OsStatus fault(km::AnyRange<size_t> range, void *base, km::MemoryRange *result) = 0;

        /// @brief Signal that a range of memory has been unmapped in a process
        virtual OsStatus release(km::AnyRange<size_t> range) = 0;

        /// @brief Flush a range of the object to the backing store
        virtual OsStatus flush(km::AnyRange<size_t> range) = 0;
    };

    class MemoryObjectRange {
        sm::RcuSharedPtr<IMemoryObject> mMemory;
        km::AnyRange<size_t> mRange;

    public:
        MemoryObjectRange(sm::RcuSharedPtr<IMemoryObject> memory, km::AnyRange<size_t> range)
            : mMemory(memory)
            , mRange(range)
        { }

        km::AnyRange<size_t> range() const noexcept { return mRange; }
        sm::RcuSharedPtr<IMemoryObject> memory() const noexcept { return mMemory; }
    };
}
