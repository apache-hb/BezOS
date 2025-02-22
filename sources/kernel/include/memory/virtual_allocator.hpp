#pragma once

#include "memory/range.hpp"
#include "memory/range_allocator.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    /// @brief Allocates virtual memory address space.
    class VirtualAllocator {
        RangeAllocator<const std::byte*> mSupervisorRanges;
        RangeAllocator<const std::byte*> mUserRanges;

    public:
        VirtualAllocator(VirtualRange range, VirtualRange user = VirtualRange{});

        void markUsed(VirtualRange range);

        void *alloc4k(size_t count, const void *hint = nullptr);
        void *alloc2m(size_t count, const void *hint = nullptr);

        void release(VirtualRange range);

        void *userAlloc4k(size_t count, const void *hint = nullptr);
        void *userAlloc2m(size_t count, const void *hint = nullptr);

        void userRelease(VirtualRange range);
    };
}
