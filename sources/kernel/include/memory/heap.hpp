#pragma once

#include "memory/detail/pool.hpp"
#include "memory/range.hpp"
#include "memory/detail/tlsf.hpp"

namespace km {
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

    class TlsfHeap {
        friend class TlsfHeapCommandList;

        using TlsfBlock = detail::TlsfBlock;
        using BlockPtr = detail::TlsfBlock*;
        using BitMap = detail::TlsfBitMap;

        /// @brief The total size of the managed area.
        size_t mSize;

        /// @brief The size of the managed area that has been marked unusable.
        size_t mReserved;
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

        void splitBlock(TlsfBlock *block, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi, TlsfBlock *newBlock) noexcept [[clang::nonallocating]];

        TlsfAllocation allocBestFit(size_t align, size_t size) [[clang::allocating]];

        TlsfHeap(PoolAllocator<TlsfBlock>&& pool, TlsfBlock *nullBlock, size_t freeListCount, std::unique_ptr<BlockPtr[]> freeList);

        /// @brief Private API for @a TlsfHeapCommandList.
        void drainBlockList(detail::TlsfBlockList list) noexcept [[clang::nonallocating]];

        /// @brief Private API for @a TlsfHeapCommandList.
        OsStatus allocateSplitBlocks(detail::TlsfBlockList& list) [[clang::allocating]];

        /// @brief Private API for @a TlsfHeapCommandList.
        OsStatus allocateSplitVectorBlocks(size_t points, detail::TlsfBlockList& list) [[clang::allocating]];

        /// @brief Private API for @a TlsfHeapCommandList.
        void commitSplitBlock(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi, detail::TlsfBlockList& list) noexcept [[clang::nonallocating]];

        /// @brief Private API for @a TlsfHeapCommandList.
        void commitSplitVectorBlocks(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results, detail::TlsfBlockList& list) noexcept [[clang::nonallocating]];

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
        OsStatus grow(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[gnu::nonnull]]) [[clang::allocating]];

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
        OsStatus shrink(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[gnu::nonnull]]) [[clang::allocating]];

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
        OsStatus resize(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[gnu::nonnull]]) [[clang::allocating]];

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
        OsStatus split(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo [[gnu::nonnull]], TlsfAllocation *hi [[gnu::nonnull]]) [[clang::allocating]];

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
        OsStatus splitv(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) [[clang::allocating]];

        [[nodiscard]]
        OsStatus reserve(MemoryRange range, TlsfAllocation *result [[gnu::nonnull]]) [[clang::allocating]];

        /// @brief Gather statistics about the heap.
        TlsfHeapStats stats() const noexcept [[clang::nonallocating]];

        /// @brief Resets the heap to its initial state.
        /// @warning Invalidates all outstanding allocations.
        void reset() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        static OsStatus create(MemoryRange range, TlsfHeap *heap [[gnu::nonnull]]) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(std::span<const MemoryRange> ranges, TlsfHeap *heap [[gnu::nonnull]]) [[clang::allocating]];
    };
}
