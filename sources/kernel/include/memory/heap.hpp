#pragma once

#include "memory/detail/pool.hpp"
#include "memory/range.hpp"

namespace km {
    static constexpr bool kEnableSplitVectorHardening = true;

    namespace detail {
        enum {
            kSmallBufferSize = 0x100,
            kMemoryClassShift = 7,
            kSecondLevelIndex = 5,
            kMaxMemoryClass = 64 - kMemoryClassShift,
            kSmallSizeStep = kSmallBufferSize / (1 << kSecondLevelIndex),
        };

        template<std::unsigned_integral T>
        static constexpr uint8_t BitScanLeading(T mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return (std::numeric_limits<T>::digits - 1) - std::countl_zero(mask);
        }

        template<std::unsigned_integral T>
        static constexpr uint8_t BitScanTrailing(T mask) {
            if (mask == 0) {
                return UINT8_MAX;
            }

            return std::countr_zero(mask);
        }

        static constexpr uint8_t SizeToMemoryClass(size_t size) {
            if (size > kSmallBufferSize) {
                return BitScanLeading(size) - kMemoryClassShift;
            }

            return 0;
        }

        static constexpr uint16_t SizeToSecondIndex(size_t size, uint8_t memoryClass) {
            [[assume(size > 0)]];

            if (memoryClass == 0) {
                return uint16_t((size - 1) / 8);
            } else {
                return uint16_t((size >> (memoryClass + kMemoryClassShift - kSecondLevelIndex)) ^ (1u << kSecondLevelIndex));
            }
        }

        static constexpr size_t GetFreeListSize(uint8_t memoryClass, uint16_t secondIndex) {
            if (memoryClass == 0) {
                return 0;
            }

            return (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex + 1 + (1 << kSecondLevelIndex);
        }

        static constexpr size_t GetListIndex(uint8_t memoryClass, uint16_t secondIndex) {
            if (memoryClass == 0) {
                return secondIndex;
            }

            size_t index = (memoryClass - 1) * (1 << kSecondLevelIndex) + secondIndex;
            return index + (1 << kSecondLevelIndex);
        }

        static constexpr size_t GetListIndex(size_t size) {
            [[assume(size > 0)]];

            uint8_t memoryClass = SizeToMemoryClass(size);
            uint16_t secondIndex = SizeToSecondIndex(size, memoryClass);
            return GetListIndex(memoryClass, secondIndex);
        }

        static constexpr size_t GetNextBlockSize(size_t size) {
            if (size > kSmallBufferSize) {
                return 1zu << (BitScanLeading(size) - kSecondLevelIndex);
            } else if (size > (kSmallBufferSize - kSmallSizeStep)) {
                return kSmallBufferSize + 1;
            } else {
                return size + kSmallSizeStep;
            }
        }

        struct TlsfBlock {
            size_t offset;
            size_t size;
            TlsfBlock *prev;
            TlsfBlock *next;
            TlsfBlock *nextFree;
            TlsfBlock *prevFree;

            bool isFree() const noexcept [[clang::nonblocking]] {
                return (prevFree != this);
            }

            bool isUsed() const noexcept [[clang::nonblocking]] {
                return (prevFree == this);
            }

            void markTaken() noexcept [[clang::nonblocking]] {
                prevFree = this;
            }

            void markFree() noexcept [[clang::nonblocking]] {
                prevFree = nullptr;
            }

            TlsfBlock *&propNextFree() noexcept [[clang::nonblocking]] {
                KM_ASSERT(isFree());
                return nextFree;
            }
        };

        using TlsfBitMap = uint32_t;
    }

    class TlsfAllocation;
    class TlsfHeap;

    struct TlsfHeapStats {
        PoolAllocatorStats pool;
        size_t freeListSize;
        size_t usedMemory;
        size_t freeMemory;
        size_t blockCount;
        size_t mallocCount;
        size_t freeCount;

        constexpr size_t controlMemory() const noexcept [[clang::nonblocking]] {
            return (sizeof(detail::TlsfBlock*) * freeListSize) + (sizeof(detail::TlsfBlock) * blockCount);
        }
    };

    struct TlsfCompactStats {
        PoolCompactStats pool;
    };

    class [[nodiscard]] TlsfAllocation {
        friend class TlsfHeap;

        detail::TlsfBlock *block{nullptr};

        TlsfAllocation(detail::TlsfBlock *block) noexcept [[clang::nonblocking]];

    public:
        constexpr TlsfAllocation() noexcept = default;

        constexpr detail::TlsfBlock *getBlock() const noexcept [[clang::nonblocking]] { return block; }
        constexpr bool isNull() const noexcept [[clang::nonblocking]] { return block == nullptr; }
        constexpr bool isValid() const noexcept [[clang::nonblocking]] { return block != nullptr; }

        constexpr size_t size() const noexcept [[clang::nonblocking]] {
            return block->size;
        }

        constexpr PhysicalAddress address() const noexcept [[clang::nonblocking]] {
            return PhysicalAddress(block->offset);
        }

        constexpr MemoryRange range() const noexcept [[clang::nonblocking]] {
            return MemoryRange::of(block->offset, block->size);
        }

        constexpr auto operator<=>(const TlsfAllocation&) const noexcept [[clang::nonblocking]] = default;
    };

    class TlsfHeap {
        using TlsfBlock = detail::TlsfBlock;
        using BlockPtr = detail::TlsfBlock*;
        using BitMap = detail::TlsfBitMap;

        size_t mSize;
        size_t mMallocCount;
        size_t mFreeCount;

        PoolAllocator<TlsfBlock> mBlockPool;
        TlsfBlock *mNullBlock;
        size_t mFreeListCount;
        std::unique_ptr<BlockPtr[]> mFreeList;

        BitMap mInnerFreeMap[detail::kMaxMemoryClass];
        BitMap mTopLevelFreeMap;

        TlsfBlock *findFreeBlock(size_t size, size_t *listIndex) noexcept [[clang::nonblocking]];

        bool checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, TlsfAllocation *result) [[clang::allocating]];

        /// @note If allocation fails, internal data structures are not modified.
        bool reserveBlock(TlsfBlock *block, size_t size, size_t alignedOffset, size_t listIndex, TlsfAllocation *result) noexcept [[clang::allocating]];

        /// @brief Detach a block from its siblings to prepare for allocation.
        void detachBlock(TlsfBlock *block, size_t listIndex) noexcept [[clang::nonblocking]];

        void takeBlockFromFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]];
        void releaseBlockToFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]];
        void mergeBlock(TlsfBlock *block, TlsfBlock *prev) noexcept [[clang::nonallocating]];

        void resizeBlock(TlsfBlock *block, size_t newSize) noexcept [[clang::nonallocating]];

        void init() noexcept;

        OsStatus addPool(MemoryRange range) [[clang::allocating]];

        void splitBlock(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi, TlsfBlock *newBlock);

        TlsfAllocation allocBestFit(size_t align, size_t size) [[clang::allocating]];

        TlsfHeap(PoolAllocator<TlsfBlock>&& pool, TlsfBlock *nullBlock, size_t freeListCount, std::unique_ptr<BlockPtr[]> freeList);

    public:
        UTIL_NOCOPY(TlsfHeap);
        UTIL_DEFAULT_MOVE(TlsfHeap);

        constexpr TlsfHeap() noexcept = default;

        void validate() noexcept [[clang::nonallocating]];

        TlsfCompactStats compact();

        [[nodiscard]]
        TlsfAllocation malloc(size_t size) [[clang::allocating]];

        /// @brief Deleted realloc for documentation.
        ///
        /// Realloc cannot be implemented, the underlying memory may not be acessible. Therefore
        /// the api contract of realloc cannot be fulfilled. See @ref shrink and @ref grow for
        /// alternatives.
        TlsfAllocation realloc(TlsfAllocation ptr, size_t size) = delete("realloc is not supported");

        /// @brief Grow an allocation to a larger size.
        ///
        /// Grows an allocation to a larger size if there is adjacent space available. If there
        /// is no adjacent space available, the allocation fails and the original
        /// allocation is unchanged.
        ///
        /// @param ptr The allocation to grow.
        /// @param size The new size of the allocation.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus grow(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]];

        /// @brief Shrink an allocation to a smaller size.
        ///
        /// Shrinks an allocation to a smaller size. If control structures cannot be
        /// allocated, the allocation fails and the original allocation is unchanged.
        ///
        /// @param ptr The allocation to shrink.
        /// @param size The new size of the allocation.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus shrink(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]];

        /// @brief Resize an allocation to a new size.
        ///
        /// Resizes an allocation to a new size using adjacent space. If this operation
        /// fails the original allocation is unchanged.
        ///
        /// @param ptr The allocation to resize.
        /// @param size The new size of the allocation.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus resize(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]];

        /// @brief Allocate an aligned allocation.
        [[nodiscard]]
        TlsfAllocation aligned_alloc(size_t align, size_t size) [[clang::allocating]];

        /// @brief Free an allocation.
        void free(TlsfAllocation ptr) noexcept [[clang::nonallocating]];

        /// @brief Split an allocation into two allocations.
        ///
        /// Splits an allocation into two allocations at the specified midpoint. The
        /// original allocation is shrunk and the second allocation covers the remaining
        /// space. The first allocation is returned in @p lo and second allocation is returned in @p hi
        ///
        /// @param ptr The allocation to split.
        /// @param midpoint The address to split the allocation at.
        /// @param[out] lo The first allocation.
        /// @param[out] hi The second allocation.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus split(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi) [[clang::allocating]];

        /// @brief Split an allocation many times.
        ///
        /// @pre @p points must not empty, must be sorted in ascending order, contain no duplicate elements,
        ///      and must contain no points outside the allocation.
        /// @pre @p results must have a size of at least `points.size() + 1`
        ///
        /// @post If the operation fails the contents of @p results is undefined.
        /// @post If the operation succeeds the original allocation is invalidated.
        ///
        /// @param ptr The allocation to split.
        /// @param points The list of points to split at.
        /// @param results Storage for the resulting split pointers.
        ///
        /// @return OsStatusSuccess if the operation is successful, otherwise a status code
        [[nodiscard]]
        OsStatus splitv(TlsfAllocation ptr, std::span<PhysicalAddress> points, std::span<TlsfAllocation> results) [[clang::allocating]];

        /// @brief Gather statistics about the heap.
        TlsfHeapStats stats() const noexcept [[clang::nonallocating]];

        /// @brief Resets the heap to its initial state.
        /// @warning Invalidates all outstanding allocations.
        void reset() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        static OsStatus create(MemoryRange range, TlsfHeap *heap) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(std::span<const MemoryRange> ranges, TlsfHeap *heap) [[clang::allocating]];
    };
}
