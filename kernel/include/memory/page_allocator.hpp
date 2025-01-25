#pragma once

#include "memory/layout.hpp"

namespace km {
    namespace detail {
        size_t GetRangeBitmapSize(MemoryRange range);
    }

    enum class PageFlags {
        eNone = 0,
        eRead = 1 << 0,
        eWrite = 1 << 1,
        eExecute = 1 << 2,
        eUser = 1 << 3,

        eCode = eRead | eExecute,
        eData = eRead | eWrite,
        eAll = eRead | eWrite | eExecute,
        eAllUser = eRead | eWrite | eExecute | eUser,
    };

    UTIL_BITFLAGS(PageFlags);

    class RegionBitmapAllocator {
        MemoryRange mRange;
        uint8_t *mBitmap;

        size_t bitCount() const;

        bool test(size_t bit) const;
        void set(size_t bit);
        void clear(size_t bit);

    public:
        /// @brief Construct a new region bitmap allocator.
        ///
        /// @pre The range must be page aligned.
        /// @pre The bitmap must be large enough to cover the range.
        ///
        /// @param range The range of memory to manage.
        /// @param bitmap The bitmap to use.
        RegionBitmapAllocator(MemoryRange range, uint8_t *bitmap);

        RegionBitmapAllocator() = default;

        PhysicalAddress alloc4k(size_t count);

        /// @brief Release a range of memory.
        ///
        /// @pre The range must have been allocated by this allocator.
        /// @pre The range must be page aligned.
        ///
        /// @param range The range to release.
        void release(MemoryRange range);

        /// @brief Get the range of memory managed by this allocator.
        ///
        /// @return The range of memory.
        MemoryRange range() const {
            return mRange;
        }

        void extend(const RegionBitmapAllocator& other) {
            mRange = km::merge(mRange, other.mRange);
            mBitmap = std::min(mBitmap, other.mBitmap);
        }

        /// @brief Mark a range of memory as used.
        ///
        /// @param range The range to mark as used.
        void markUsed(MemoryRange range);
    };

    namespace detail {
        using RegionList = stdx::Vector<RegionBitmapAllocator>;
        using RegionFreeList = stdx::Vector<MemoryRange>;

        void BuildMemoryRanges(
            RegionList& allocators, RegionList& lowMemory,
            const boot::MemoryMap& layout, uint8_t *bitmap
        );

        void MergeAdjacentAllocators(RegionList& allocators);
    }

    class PageAllocator {
        detail::RegionFreeList mMemoryFree;
        detail::RegionFreeList mLowMemoryFree;

        /// @brief One allocator for each usable or reclaimable memory range.
        detail::RegionList mAllocators;

        /// @brief One allocator for each memory range below 1M.
        detail::RegionList mLowMemory;

        std::unique_ptr<uint8_t[], mem::AllocatorDeleter<uint8_t>> mBitmapMemory;

    public:
        PageAllocator(const boot::MemoryMap& memmap, mem::IAllocator *allocator);

        /// @brief Rebuild the internal allocators.
        ///
        /// Must be called once bootloader memory has been reclaimed.
        /// This will merge any internal allocators that are now contiguous.
        void rebuild();

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
