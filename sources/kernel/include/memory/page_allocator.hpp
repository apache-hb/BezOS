#pragma once

#include "boot.hpp"

#include "memory/heap.hpp"
#include "common/compiler/compiler.hpp"
#include "memory/pmm_heap.hpp"
#include "std/spinlock.hpp"

namespace km {
    struct PageAllocatorStats {
        TlsfHeapStats heap;
    };

    class PageAllocator {
        friend class PageAllocatorCommandList;

        stdx::SpinLock mLock;

        km::PmmHeap mMemoryHeap GUARDED_BY(mLock);

    public:
        UTIL_NOCOPY(PageAllocator);

        constexpr PageAllocator(PageAllocator&& other) noexcept
            : mMemoryHeap(std::move(other.mMemoryHeap))
        { }

        constexpr PageAllocator& operator=(PageAllocator&& other) noexcept {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

            mMemoryHeap = std::move(other.mMemoryHeap);
            return *this;

            CLANG_DIAGNOSTIC_POP();
        }

        constexpr PageAllocator() noexcept = default;

        /// @brief Allocate a number of contiguous physical pages.
        ///
        /// @param count The number of contiguous pages to allocate.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        PmmAllocation pageAlloc(size_t count = 1) [[clang::allocating]];

        PmmAllocation aligned_alloc(size_t align, size_t size) [[clang::allocating]];

        [[nodiscard]]
        OsStatus splitv(PmmAllocation ptr, std::span<const PhysicalAddress> points, std::span<PmmAllocation> results);

        [[nodiscard]]
        OsStatus split(PmmAllocation allocation, PhysicalAddress midpoint, PmmAllocation *lo [[outparam]], PmmAllocation *hi [[outparam]]) [[clang::allocating]];

        void free(PmmAllocation allocation) noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus reserve(MemoryRange range, PmmAllocation *allocation [[outparam]]) [[clang::allocating]];

        /// @brief Release a range of memory.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        PageAllocatorStats stats() noexcept;

        [[nodiscard]]
        static OsStatus create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator [[outparam]]) [[clang::allocating]];
    };
}
