#pragma once

#include "std/std.hpp"
#include "memory/range.hpp"
#include "memory/detail/pool.hpp"
#include "memory/detail/tlsf.hpp" // IWYU pragma: export

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

        TlsfBlock *findFreeBlock(size_t size, size_t *listIndex) const noexcept [[clang::nonblocking]];

        bool checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, TlsfAllocation *result) [[clang::allocating]];

        /// @note If allocation fails, internal data structures are not modified.
        bool reserveBlock(TlsfBlock *block, size_t size, size_t alignedOffset, size_t listIndex, TlsfAllocation *result) noexcept [[clang::allocating]];

        /// @brief Detach a block from its siblings to prepare for allocation.
        void detachBlock(TlsfBlock *block, size_t listIndex) noexcept [[clang::nonblocking]];

        void takeBlockFromFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]];
        void releaseBlockToFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]];
        void mergeBlock(TlsfBlock *block, TlsfBlock *prev) noexcept [[clang::nonallocating]];

        void resizeBlock(TlsfBlock *block, size_t newSize) noexcept [[clang::nonallocating]];

        OsStatus splitBlockForAllocationIfRequired(TlsfBlock *block, size_t alignedOffset) noexcept [[clang::allocating]];

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

        /// @brief Validate internal data structures.
        ///
        /// @note This is an expensive operation and should only be used for debugging.
        void validate() noexcept [[clang::nonallocating]];

        /// @brief Compact internal data structures.
        ///
        /// @return Statistics about the compaction operation.
        TlsfCompactStats compact();

        /// @brief Allocate a block of memory.
        ///
        /// Equivalent to `aligned_alloc(alignof(std::max_align_t), size)`.
        ///
        /// @param size The size of the block to allocate.
        ///
        /// @return The allocation, or a null allocation on failure.
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
        /// allocation is unchanged. Cannot be used to shrink allocations, see @ref shrink.
        ///
        /// @param ptr The allocation to grow.
        /// @param size The new size of the allocation.
        ///
        /// @return The status of the operation.
        /// @retval OsStatusSuccess The operation was successful, @p result contains the new allocation.
        /// @retval OsStatusInvalidInput The new size is less than the current size.
        /// @retval OsStatusOutOfMemory There is no adjacent space available, the original allocation is unchanged.
        [[nodiscard]]
        OsStatus grow(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[outparam]]) [[clang::allocating]];

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
        OsStatus shrink(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[outparam]]) [[clang::allocating]];

        /// @brief Resize an allocation to a new size.
        ///
        /// Resizes an allocation to a new size using adjacent space. If this operation
        /// fails the original allocation is unchanged.
        ///
        /// @param ptr The allocation to resize.
        /// @param size The new size of the allocation.
        /// @param[out] result The resulting allocation if successful, otherwise unchanged.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus resize(TlsfAllocation ptr, size_t size, TlsfAllocation *result [[outparam]]) [[clang::allocating]];

        /// @brief Allocate an aligned allocation.
        ///
        /// @param align The alignment of the allocation.
        /// @param size The size of the allocation.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        TlsfAllocation aligned_alloc(size_t align, size_t size) [[clang::allocating]];

        /// @brief Allocate a block of memory with a preferred address.
        ///
        /// @param align The alignment of the allocation.
        /// @param size The size of the allocation.
        /// @param hint The preferred address of the allocation, may be ignored if the address is not available.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        TlsfAllocation allocateWithHint(size_t align, size_t size, uintptr_t hint) [[clang::allocating]];

        /// @brief Allocate a block of memory at a specific address.
        ///
        /// Like @ref allocateWithHint, but the address is mandatory. The allocation
        /// will fail if the address is not available.
        ///
        /// @param address The address to allocate the memory at.
        /// @param size The size of the allocation.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        TlsfAllocation allocateAt(uintptr_t address, size_t size) [[clang::allocating]];

        /// @brief Free an allocation.
        ///
        /// @param ptr The allocation to free.
        void free(TlsfAllocation ptr) noexcept [[clang::nonallocating]];

        /// @brief Free an allocation by its address.
        /// @warning If you're using this function, you're doing it wrong.
        void freeAddress(PhysicalAddress address) noexcept [[clang::nonallocating]];

        /// @brief Find an allocation by its address.
        /// @warning If you're using this function, you're doing it wrong.
        OsStatus findAllocation(PhysicalAddress address, TlsfAllocation *result [[outparam]]) noexcept [[clang::nonallocating]];

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
        OsStatus split(TlsfAllocation ptr, uintptr_t midpoint, TlsfAllocation *lo [[outparam]], TlsfAllocation *hi [[outparam]]) [[clang::allocating]];

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

        /// @brief Reserve a range of memory.
        ///
        /// @param range The range to reserve.
        /// @param[out] result The resulting allocation if successful, required to release the memory later.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, memory was reserved.
        /// @retval OsStatusOutOfMemory Failed to allocate control structures, no memory was reserved.
        /// @retval OsStatusNotAvailable The range is already in use, no memory was reserved.
        /// @retval OsStatusNotFound The range is not managed by this heap, no memory was reserved.
        [[nodiscard]]
        OsStatus reserve(MemoryRange range, TlsfAllocation *result [[outparam]]) [[clang::allocating]];

        /// @brief Gather statistics about the heap.
        ///
        /// @return The current heap statistics.
        [[nodiscard]]
        TlsfHeapStats stats() const noexcept [[clang::nonallocating]];

        /// @brief Resets the heap to its initial state.
        /// @warning Invalidates all outstanding allocations.
        void reset() noexcept [[clang::nonallocating]];

        /// @brief Create a heap from a single contiguous range.
        ///
        /// @param range The range to create the heap from.
        /// @param[out] heap The resulting heap.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, the heap was created.
        [[nodiscard]]
        static OsStatus create(MemoryRange range, TlsfHeap *heap [[outparam]]) [[clang::allocating]];

        /// @brief Create a heap from multiple discontiguous ranges.
        ///
        /// @pre @p ranges must not be empty, and must be sorted in ascending order.
        ///
        /// @param ranges The ranges to create the heap from.
        /// @param[out] heap The resulting heap.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, the heap was created.
        [[nodiscard]]
        static OsStatus create(std::span<const MemoryRange> ranges, TlsfHeap *heap [[outparam]]) [[clang::allocating]];
    };

    template<typename T>
    class GenericTlsfHeap {
        static_assert(sizeof(T) == sizeof(uintptr_t), "T must be the same size as an address");

        TlsfHeap mHeap;

    public:
        using Self = GenericTlsfHeap<T>;
        using Address = T;
        using Range = AnyRange<T>;
        using Allocation = GenericTlsfAllocation<Address>;

        UTIL_NOCOPY(GenericTlsfHeap);
        UTIL_DEFAULT_MOVE(GenericTlsfHeap);

        constexpr GenericTlsfHeap() noexcept = default;

        /// @brief Allocate a block of memory.
        ///
        /// Equivalent to `alignedAlloc(alignof(std::max_align_t), size)`.
        ///
        /// @param size The size of the block to allocate.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        Allocation malloc(size_t size) [[clang::allocating]] {
            return Allocation{mHeap.malloc(size).getBlock()};
        }

        /// @brief Deleted realloc for documentation.
        ///
        /// Realloc cannot be implemented, the underlying memory may not be acessible. Therefore
        /// the api contract of realloc cannot be fulfilled. See @ref shrink and @ref grow for
        /// alternatives.
        Allocation realloc(Allocation ptr, size_t size) = delete("realloc is not supported");

        /// @brief Grow an allocation to a larger size.
        ///
        /// Grows an allocation to a larger size if there is adjacent space available. If there
        /// is no adjacent space available, the allocation fails and the original
        /// allocation is unchanged. Cannot be used to shrink allocations, see @ref shrink.
        ///
        /// @param ptr The allocation to grow.
        /// @param size The new size of the allocation.
        ///
        /// @return The status of the operation.
        /// @retval OsStatusSuccess The operation was successful, @p result contains the new allocation.
        /// @retval OsStatusInvalidInput The new size is less than the current size.
        /// @retval OsStatusOutOfMemory There is no adjacent space available, the original allocation is unchanged.
        [[nodiscard]]
        OsStatus grow(Allocation ptr, size_t size, Allocation *result [[outparam]]) [[clang::allocating]] {
            TlsfAllocation out;
            if (OsStatus status = mHeap.grow(TlsfAllocation{ptr.getBlock()}, size, &out)) {
                return status;
            }

            *result = Allocation{out.getBlock()};
            return OsStatusSuccess;
        }

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
        OsStatus shrink(Allocation ptr, size_t size, Allocation *result [[outparam]]) [[clang::allocating]] {
            TlsfAllocation out;
            if (OsStatus status = mHeap.shrink(TlsfAllocation{ptr.getBlock()}, size, &out)) {
                return status;
            }

            *result = Allocation{out.getBlock()};
            return OsStatusSuccess;
        }

        /// @brief Resize an allocation to a new size.
        ///
        /// Resizes an allocation to a new size using adjacent space. If this operation
        /// fails the original allocation is unchanged.
        ///
        /// @param ptr The allocation to resize.
        /// @param size The new size of the allocation.
        /// @param[out] result The resulting allocation if successful, otherwise unchanged.
        ///
        /// @return The status of the operation.
        [[nodiscard]]
        OsStatus resize(Allocation ptr, size_t size, Allocation *result [[outparam]]) [[clang::allocating]] {
            TlsfAllocation out;
            if (OsStatus status = mHeap.resize(TlsfAllocation{ptr.getBlock()}, size, &out)) {
                return status;
            }

            *result = Allocation{out.getBlock()};
            return OsStatusSuccess;
        }

        /// @brief Allocate an aligned allocation.
        ///
        /// @param align The alignment of the allocation.
        /// @param size The size of the allocation.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        Allocation alignedAlloc(size_t align, size_t size) [[clang::allocating]] {
            return Allocation{mHeap.aligned_alloc(align, size).getBlock()};
        }

        /// @brief Allocate a block of memory with a preferred address.
        ///
        /// @param align The alignment of the allocation.
        /// @param size The size of the allocation.
        /// @param hint The preferred address of the allocation, may be ignored if the address is not available.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        Allocation allocateWithHint(size_t align, size_t size, Address hint) [[clang::allocating]] {
            return Allocation{mHeap.allocateWithHint(align, size, hint.address).getBlock()};
        }

        /// @brief Allocate a block of memory at a specific address.
        ///
        /// Like @ref allocateWithHint, but the address is mandatory. The allocation
        /// will fail if the address is not available.
        ///
        /// @param address The address to allocate the memory at.
        /// @param size The size of the allocation.
        ///
        /// @return The allocation, or a null allocation on failure.
        [[nodiscard]]
        Allocation allocateAt(Address address, size_t size) [[clang::allocating]] {
            return Allocation{mHeap.allocateAt(address.address, size).getBlock()};
        }

        /// @brief Free an allocation.
        ///
        /// @param ptr The allocation to free.
        void free(Allocation ptr) noexcept [[clang::nonallocating]] {
            mHeap.free(TlsfAllocation{ptr.getBlock()});
        }

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
        OsStatus split(Allocation ptr, Address midpoint, Allocation *lo [[outparam]], Allocation *hi [[outparam]]) [[clang::allocating]] {
            TlsfAllocation outLo;
            TlsfAllocation outHi;
            if (OsStatus status = mHeap.split(TlsfAllocation{ptr.getBlock()}, midpoint.address, &outLo, &outHi)) {
                return status;
            }

            *lo = Allocation{outLo.getBlock()};
            *hi = Allocation{outHi.getBlock()};
            return OsStatusSuccess;
        }

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
        OsStatus splitv(Allocation ptr, std::span<const Address> points, std::span<Allocation> results) [[clang::allocating]] {
            const PhysicalAddress *ptrIn = std::bit_cast<const PhysicalAddress*>(points.data());
            std::span<const PhysicalAddress> p { ptrIn, points.size() };

            TlsfAllocation *resIn = std::bit_cast<TlsfAllocation*>(results.data());
            std::span<TlsfAllocation> r { resIn, results.size() };
            return mHeap.splitv(TlsfAllocation{ptr.getBlock()}, p, r);
        }

        /// @brief Reserve a range of memory.
        ///
        /// @param range The range to reserve.
        /// @param[out] result The resulting allocation if successful, required to release the memory later.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, memory was reserved.
        /// @retval OsStatusOutOfMemory Failed to allocate control structures, no memory was reserved.
        /// @retval OsStatusNotAvailable The range is already in use, no memory was reserved.
        /// @retval OsStatusNotFound The range is not managed by this heap, no memory was reserved.
        [[nodiscard]]
        OsStatus reserve(Range range, Allocation *result [[outparam]]) [[clang::allocating]] {
            TlsfAllocation out;
            MemoryRange r { range.front.address, range.back.address };
            if (OsStatus status = mHeap.reserve(r, &out)) {
                return status;
            }

            *result = Allocation{out.getBlock()};
            return OsStatusSuccess;
        }

        /// @brief Find an allocation by its address.
        /// @warning If you're using this function, you're doing it wrong.
        OsStatus findAllocation(Address address, Allocation *result [[outparam]]) noexcept [[clang::nonallocating]] {
            TlsfAllocation out;
            if (OsStatus status = mHeap.findAllocation(PhysicalAddress{address.address}, &out)) {
                return status;
            }

            *result = Allocation{out.getBlock()};
            return OsStatusSuccess;
        }

        /// @brief Validate internal data structures.
        ///
        /// @note This is an expensive operation and should only be used for debugging.
        void validate() noexcept [[clang::nonallocating]] {
            mHeap.validate();
        }

        /// @brief Compact internal data structures.
        ///
        /// @return Statistics about the compaction operation.
        TlsfCompactStats compact() {
            return mHeap.compact();
        }

        /// @brief Gather statistics about the heap.
        ///
        /// @return The current heap statistics.
        [[nodiscard]]
        TlsfHeapStats stats() const noexcept [[clang::nonallocating]] {
            return mHeap.stats();
        }

        /// @brief Resets the heap to its initial state.
        ///
        /// @warning Invalidates all outstanding allocations.
        void reset() noexcept [[clang::nonallocating]] {
            mHeap.reset();
        }

        /// @brief Create a heap from a single contiguous range.
        ///
        /// @param range The range to create the heap from.
        /// @param[out] heap The resulting heap.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, the heap was created.
        [[nodiscard]]
        static OsStatus create(Range range, Self *heap [[outparam]]) [[clang::allocating]] {
            MemoryRange r { range.front.address, range.back.address };
            return TlsfHeap::create(r, &heap->mHeap);
        }

        /// @brief Create a heap from multiple discontiguous ranges.
        ///
        /// @pre @p ranges must not be empty, and must be sorted in ascending order.
        ///
        /// @param ranges The ranges to create the heap from.
        /// @param[out] heap The resulting heap.
        ///
        /// @return The result of the operation.
        /// @retval OsStatusSuccess The operation was successful, the heap was created.
        [[nodiscard]]
        static OsStatus create(std::span<const Range> ranges, Self *heap [[outparam]]) [[clang::allocating]] {
            //
            // TODO: I'm not sure if this is technically allowed, but the layouts are the same
            // for all ranges so this should be safe.
            //
            const MemoryRange *ptr = std::bit_cast<const MemoryRange*>(ranges.data());
            std::span<const MemoryRange> r { ptr, ranges.size() };
            return TlsfHeap::create(r, &heap->mHeap);
        }
    };
}
