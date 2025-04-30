#pragma once

#include "boot.hpp"

#include "memory/heap.hpp"
#include "memory/range_allocator.hpp"

namespace km {
    struct PageAllocatorStats {
        size_t freeMemory;
        size_t freeLowMemory;
    };

    class PageAllocator {
        /// @brief Allocator for memory below 1M.
        RangeAllocator<PhysicalAddress> mLowMemory;

        /// @brief Allocator for usable memory above 1M.
        RangeAllocator<PhysicalAddress> mMemory;

        km::TlsfHeap mMemoryHeap;

    public:
        UTIL_NOCOPY(PageAllocator);
        UTIL_DEFAULT_MOVE(PageAllocator);

        constexpr PageAllocator() noexcept = default;

        PageAllocator(std::span<const boot::MemoryRegion> memmap);

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        MemoryRange alloc4k(size_t count = 1) [[clang::allocating]];

        TlsfAllocation pageAlloc(size_t count = 1) [[clang::allocating]];

        void release(TlsfAllocation allocation) noexcept [[clang::nonallocating]];

        /// @brief Release a range of memory.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        /// @brief Allocate a 4k page of memory below 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress lowMemoryAlloc4k() [[clang::allocating]];

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void reserve(MemoryRange range);

        PageAllocatorStats stats() {
            return { mMemory.freeSpace(), mLowMemory.freeSpace() };
        }

        [[nodiscard]]
        static OsStatus create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator) [[clang::allocating]];
    };
}
