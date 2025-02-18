#pragma once

#include "memory/range.hpp"
#include "memory/range_allocator.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    /// @brief Allocates virtual memory address space.
    class VirtualAllocator {
        RangeAllocator<const void*> mSupervisorRanges;
        RangeAllocator<const void*> mUserRanges;

    public:
        VirtualAllocator(VirtualRange range, VirtualRange user = VirtualRange{});

        void markUsed(VirtualRange range);

        void *alloc4k(size_t count);
        void *alloc2m(size_t count);

        void release(VirtualRange range);

        void *userAlloc4k(size_t count);
        void *userAlloc2m(size_t count);

        void userRelease(VirtualRange range);
    };
}
