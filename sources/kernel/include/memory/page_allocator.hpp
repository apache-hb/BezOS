#pragma once

#include "boot.hpp"

#include "memory/heap.hpp"
#include "common/compiler/compiler.hpp"
#include "std/spinlock.hpp"

namespace km {
    struct PageAllocatorStats {
        TlsfHeapStats heap;
    };

    class PageAllocator {
        friend class PageAllocatorCommandList;

        stdx::SpinLock mLock;

        km::TlsfHeap mMemoryHeap GUARDED_BY(mLock);

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

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        MemoryRange alloc4k(size_t count = 1) [[clang::allocating]];

        TlsfAllocation pageAlloc(size_t count = 1) [[clang::allocating]];

        TlsfAllocation aligned_alloc(size_t align, size_t size) [[clang::allocating]];

        [[nodiscard]]
        OsStatus splitv(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results);

        [[nodiscard]]
        OsStatus split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo [[gnu::nonnull]], TlsfAllocation *hi [[gnu::nonnull]]) [[clang::allocating]];

        void free(TlsfAllocation allocation) noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus reserve(MemoryRange range, TlsfAllocation *allocation [[gnu::nonnull]]) [[clang::allocating]];

        /// @brief Release a range of memory.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        PageAllocatorStats stats() noexcept;

        [[nodiscard]]
        static OsStatus create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator [[gnu::nonnull]]) [[clang::allocating]];
    };
}
