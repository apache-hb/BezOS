#pragma once

#include "boot.hpp"

#include "memory/range_allocator.hpp"

namespace km {
    class PageAllocator {
        /// @brief Allocator for memory below 1M.
        RangeAllocator<PhysicalAddress> mLowMemory;

        /// @brief Allocator for usable memory above 1M.
        RangeAllocator<PhysicalAddress> mMemory;

    public:
        PageAllocator(std::span<const boot::MemoryRegion> memmap);

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress alloc4k(size_t count = 1);

        /// @brief Release a range of memory.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        /// @brief Allocate a 4k page of memory below 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress lowMemoryAlloc4k();

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void markUsed(MemoryRange range);
    };
}
