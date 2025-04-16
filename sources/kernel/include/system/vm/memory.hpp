#pragma once

#include "system/vmem.hpp"
#include "std/vector.hpp"

namespace sys2 {
    struct MemorySegment {
        std::atomic<uint32_t> mOwners;
        km::MemoryRange mRange;
    };

    /// @brief Physical storage for vm objects.
    ///
    /// This contains physical memory that a vm segment maps onto,
    /// multiple vm objects may map to the same memory segment.
    /// This object can be referenced from multiple processes at the same time.
    class MemoryObject : public IMemoryObject {
        stdx::Vector<MemorySegment> mSegments;

    public:
        OsStatus fault(km::AnyRange<size_t> range, void *base, km::MemoryRange *result) override;

        OsStatus release(km::AnyRange<size_t> range) override;

        OsStatus flush(km::AnyRange<size_t> range) override;
    };
}
