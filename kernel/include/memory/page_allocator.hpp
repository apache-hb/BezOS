#pragma once

#include "memory/layout.hpp"

#include "util/digit.hpp"

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
        using RegionAllocators = stdx::StaticVector<RegionBitmapAllocator, SystemMemoryLayout::kMaxRanges>;
        using LowMemoryAllocators = stdx::StaticVector<RegionBitmapAllocator, 4>;

        void BuildMemoryRanges(
            RegionAllocators& allocators, LowMemoryAllocators& lowMemory,
            const SystemMemoryLayout *layout, PhysicalAddress bitmap, uintptr_t hhdmOffset
        );

        template<size_t N>
        void MergeAdjacentAllocators(stdx::StaticVector<RegionBitmapAllocator, N>& allocators) {
            for (size_t i = 0; i < allocators.count(); i++) {
                RegionBitmapAllocator& allocator = allocators[i];
                for (size_t j = i + 1; j < allocators.count(); j++) {
                    RegionBitmapAllocator& next = allocators[j];
                    if (allocator.range().intersects(next.range())) {
                        allocator.extend(next);
                        allocators.remove(j);
                        j--;
                    }
                }
            }
        }
    }

    using sm::uint24_t;

    /// @brief A range of free pages.
    ///
    /// Deriving the maximum size of a memory range.
    ///
    /// let sw = sizeof(FreeRange::start) * CHAR_BIT
    /// let pg = x64::kPageSize
    /// let mr = ((1 << sw) - 1) * pg
    /// mr = 0xF'FFFF'F000 bytes or 63.999GB per free range.
    ///
    /// The worst case free range allocation size can be dervied from sizeof(FreeRange) * (mr / 2).
    /// Which describes the case of every other page being free.
    /// wc = 0x1FF'FFFC bytes or 33.55MB of overhead in the worst case.
    /// This is a 0.048% worst case memory usage overhead, I feel this is good enough.
    struct FreeRange {
        uint24_t start;
        uint8_t count;
    };

    constexpr size_t kMaxPageAllocatorSize = std::numeric_limits<uint24_t>::max() * x64::kPageSize;
    constexpr size_t kPageAllocatorWorstCaseSize = sizeof(FreeRange) * (std::numeric_limits<uint24_t>::max() / 2);

    class PageAllocator {
        /// @brief One allocator for each usable or reclaimable memory range.
        detail::RegionAllocators mAllocators;

        /// @brief One allocator for each memory range below 1M.
        detail::LowMemoryAllocators mLowMemory;

        MemoryRange mBitmapMemory;

    public:
        PageAllocator(const SystemMemoryLayout *layout, uintptr_t hhdmOffset);

        /// @brief Rebuild the internal allocators.
        ///
        /// Must be called once bootloader memory has been reclaimed.
        /// This will merge any internal allocators that are now contiguous.
        void rebuild();

        /// @brief Allocate a 4k page of memory above 1M.
        ///
        /// @return The physical address of the page.
        PhysicalAddress alloc4k(size_t count = 1);

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
