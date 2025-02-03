#pragma once

#include "memory/range.hpp"
#include "memory/range_allocator.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    /// @brief Allocates virtual memory address space.
    class VirtualAllocator {
        RangeAllocator<const void*> mRangeAllocator;

    public:
        VirtualAllocator(VirtualRange range);

        void markUsed(VirtualRange range);

        void *alloc4k(size_t count);
        void *alloc2m(size_t count);

        void release(VirtualRange range);
    };
}
