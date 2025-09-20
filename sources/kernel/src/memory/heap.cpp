#include "memory/heap.hpp"
#include "logger/categories.hpp"
#include "memory/range.hpp"

#include "std/static_vector.hpp"

#include <ranges>

using km::detail::TlsfBlock;
using km::TlsfHeap;
using km::TlsfHeapStats;
using km::TlsfAllocation;

namespace {
    template<size_t N>
    struct BlockBuffer {
        km::PoolAllocator<TlsfBlock> *pool;
        stdx::StaticVector<TlsfBlock*, N> blocks;

        UTIL_NOCOPY(BlockBuffer);
        UTIL_NOMOVE(BlockBuffer);

        ~BlockBuffer() noexcept {
            for (TlsfBlock *block : blocks) {
                pool->destroy(block);
            }
        }

        BlockBuffer(km::PoolAllocator<TlsfBlock> *pool) noexcept
            : pool(pool)
        { }

        OsStatus reserveOne() [[clang::allocating]] {
            KM_CHECK(!blocks.isFull(), "Block buffer is full");
            if (TlsfBlock *block = pool->construct()) {
                blocks.add(block);
                return OsStatusSuccess;
            }

            return OsStatusOutOfMemory;
        }

        OsStatus reserve(size_t count = 1) [[clang::allocating]] {
            for (size_t i = 0; i < count; i++) {
                if (OsStatus status = reserveOne()) {
                    return status;
                }
            }

            return OsStatusSuccess;
        }

        [[gnu::returns_nonnull]]
        TlsfBlock *take(TlsfBlock init) noexcept [[clang::nonblocking]] {
            KM_CHECK(!blocks.isEmpty(), "No blocks in buffer");
            TlsfBlock *block = blocks.back();
            blocks.pop();
            *block = init;
            return block;
        }
    };
}

TlsfAllocation::TlsfAllocation(detail::TlsfBlock *block) noexcept [[clang::nonblocking]]
    : mBlock(block)
{
    if (block != nullptr) {
        KM_CHECK(!block->isFree(), "Allocation was not created from an allocated block");
    }
}

TlsfBlock *TlsfHeap::findFreeBlock(size_t size, size_t *listIndex) const noexcept [[clang::nonblocking]] {
    KM_CHECK(size > 0, "size must be greater than 0");

    uint8_t memoryClass = detail::SizeToMemoryClass(size);
    uint16_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
    if (memoryClass > std::numeric_limits<BitMap>::digits) {
        return nullptr;
    }

    BitMap innerFreeMap = mInnerFreeMap[memoryClass] & (BitMap(~0) << secondIndex);
    if (innerFreeMap == 0) {
        BitMap freeMap = mTopLevelFreeMap & (BitMap(~0) << (memoryClass + 1));
        if (freeMap == 0) {
            return nullptr;
        }

        memoryClass = detail::BitScanTrailing(freeMap);
        innerFreeMap = mInnerFreeMap[memoryClass];
        KM_ASSERT(innerFreeMap != 0);
    }

    size_t index = detail::GetListIndex(memoryClass, detail::BitScanTrailing(innerFreeMap));
    *listIndex = index;
    return mFreeList[index];
}

void TlsfHeap::init() noexcept {
    mTopLevelFreeMap = 0;
    std::uninitialized_fill_n(mFreeList.get(), mFreeListCount, nullptr);
    std::uninitialized_fill(std::begin(mInnerFreeMap), std::end(mInnerFreeMap), 0);
}

TlsfHeap::TlsfHeap(PoolAllocator<TlsfBlock>&& pool, TlsfBlock *nullBlock, size_t freeListCount, std::unique_ptr<BlockPtr[]> freeList)
    : mSize(nullBlock->size)
    , mReserved(0)
    , mMallocCount(0)
    , mFreeCount(0)
    , mBlockPool(std::move(pool))
    , mNullBlock(nullBlock)
    , mFreeListCount(freeListCount)
    , mFreeList(std::move(freeList))
{
    mNullBlock->markFree();
    init();
}

OsStatus TlsfHeap::create(MemoryRange range, TlsfHeap *heap [[clang::noescape, gnu::nonnull]]) [[clang::allocating]] {
    size_t size = range.size();
    uint8_t memoryClass = detail::SizeToMemoryClass(size);
    uint16_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
    size_t freeListCount = detail::GetFreeListSize(memoryClass, secondIndex);

    PoolAllocator<TlsfBlock> pool;
    TlsfBlock *nullBlock = pool.construct(TlsfBlock {
        .offset = range.front.address,
        .size = range.size(),
    });
    if (nullBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    BlockPtr *freeList = new (std::nothrow) BlockPtr[freeListCount];
    if (freeList == nullptr) {
        //
        // We can leak the nullBlock here, as the pool its allocated from
        // will be freed when we leave this function early.
        //
        return OsStatusOutOfMemory;
    }

    *heap = TlsfHeap(std::move(pool), nullBlock, freeListCount, std::unique_ptr<BlockPtr[]>(freeList));
    return OsStatusSuccess;
}

OsStatus TlsfHeap::create(std::span<const MemoryRange> ranges, TlsfHeap *heap [[clang::noescape, gnu::nonnull]]) [[clang::allocating]] {
    if (ranges.empty()) {
        return OsStatusInvalidInput;
    }

    if (ranges.size() == 1) {
        return create(ranges[0], heap);
    }

    KM_CHECK(std::ranges::is_sorted(ranges, std::ranges::less{}, &MemoryRange::front), "Ranges must be sorted");

    km::MemoryRange all = km::combinedInterval(ranges);

    if (OsStatus status = km::TlsfHeap::create(all, heap)) {
        return status;
    }

    size_t reserved = 0;

    //
    // Iterate over all the holes in the range and reserve them
    //
    for (size_t i = 0; i < ranges.size() - 1; i++) {
        const MemoryRange& range = ranges[i];
        const MemoryRange& next = ranges[i + 1];
        if (range.back.address < next.front.address) {
            MemoryRange hole = { range.back.address, next.front.address };

            //
            // Leaking the allocation here is fine, the memory isnt usable anyway
            // and the allocation handle will be freed when the heap is destroyed.
            //
            TlsfAllocation allocation;
            if (OsStatus status = heap->reserve(hole, &allocation)) {
                return status;
            }

            reserved += hole.size();
        }
    }

    heap->mReserved = reserved;
    heap->validate();
    return OsStatusSuccess;
}

OsStatus TlsfHeap::addPool(MemoryRange range) [[clang::allocating]] {
    if (range.isEmpty()) {
        return OsStatusInvalidInput;
    }

    BlockBuffer<2> buffer(&mBlockPool);
    if (OsStatus status = buffer.reserve(2)) {
        return status;
    }

    TlsfBlock *block = buffer.take(TlsfBlock {
        .offset = range.front.address,
        .size = range.size(),
    });

    TlsfBlock *sentinel = buffer.take(TlsfBlock {
        .offset = range.back.address,
        .size = 0,
        .prevFree = block,
    });

    block->markTaken();
    block->next = sentinel;
    mSize += range.size();

    releaseBlockToFreeList(block);
    return OsStatusSuccess;
}

void TlsfHeap::validate() noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mFreeListCount; i++) {
        TlsfBlock *head = mFreeList[i];
        TlsfBlock *block = head;
        if (block != nullptr) {
            KM_CHECK(block->isFree(), "Block in freelist is not free");
            KM_CHECK(block->prevFree == nullptr, "freelist head block has a previous free block");
            while (block->propNextFree() != nullptr) {
                KM_CHECK(block->propNextFree() != head, "Freelist is cyclic");

                KM_CHECK(block->propNextFree()->isFree(), "Block is not free");
                KM_CHECK(block->propNextFree()->prevFree == block, "Block has an incorrect previous free block");
                KM_CHECK(block->propNextFree() != block, "Block has a self reference");
                block = block->propNextFree();
            }
        }
    }

    KM_CHECK(mNullBlock->next == nullptr, "Null block has a next block");
    KM_CHECK(mNullBlock->isFree(), "Null block is not free");
    if (mNullBlock->prev != nullptr) {
        KM_CHECK(mNullBlock->prev->next == mNullBlock, "Null block previous block does not point to null block");
    }

    size_t nextOffset = mNullBlock->offset;
    size_t computedSize = mNullBlock->size;
    for (TlsfBlock *prev = mNullBlock->prev; prev != nullptr; prev = prev->prev) {
        KM_CHECK(prev->offset + prev->size == nextOffset, "Block offset does not end at the next block");
        nextOffset = prev->offset;
        computedSize += prev->size;

        size_t listIndex = detail::GetListIndex(prev->size);
        TlsfBlock *block = mFreeList[listIndex];
        if (prev->isFree()) {
            KM_CHECK(block != nullptr, "Block is not in the free list");

            bool found = false;
            do {
                if (block == prev) {
                    found = true;
                }

                block = block->propNextFree();
            } while (!found && block != nullptr);

            KM_CHECK(found, "Block is not in the free list");
        } else {
            while (block != nullptr) {
                KM_CHECK(block != prev, "Block is in the free list");
                block = block->propNextFree();
            }
        }

        if (prev->prev != nullptr) {
            KM_CHECK(prev->prev->next == prev, "Block does not point to the next block");
        }
    }

    KM_CHECK(computedSize == mSize, "Computed size does not match the expected size");
}

km::TlsfCompactStats TlsfHeap::compact() {
    TlsfBlock *block = mNullBlock;
    while (block != nullptr && block->prev != nullptr) {
        TlsfBlock *prev = block->prev;
        if (block->isFree() && prev->isFree()) {
            mergeBlock(block, prev);
        } else {
            block = prev;
        }
    }

    PoolCompactStats pool = mBlockPool.compact();
    return TlsfCompactStats { pool };
}

km::TlsfAllocation TlsfHeap::malloc(size_t size) [[clang::allocating]] {
    return aligned_alloc(alignof(std::max_align_t), size);
}

void TlsfHeap::resizeBlock(TlsfBlock *block, size_t newSize) noexcept [[clang::nonallocating]] {
    KM_CHECK(block != mNullBlock, "Cannot resize null block");

    size_t oldListIndex = detail::GetListIndex(block->size);
    size_t newListIndex = detail::GetListIndex(newSize);
    if (oldListIndex != newListIndex) {
        takeBlockFromFreeList(block);
        block->size = newSize;
        releaseBlockToFreeList(block);
    }
}

OsStatus TlsfHeap::grow(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]] {
    KM_CHECK(ptr.isValid(), "Invalid address");
    TlsfBlock *block = ptr.getBlock();
    KM_CHECK(!block->isFree(), "Block is not free");

    //
    // If the size is the same, just return the same allocation.
    //
    if (size == block->size) {
        *result = ptr;
        return OsStatusSuccess;
    }

    //
    // This function only grows allocations, shrinking should be done with shrink().
    //
    if (size < block->size) {
        return OsStatusInvalidInput;
    }

    //
    // Try to expand the block into the adjacent block if it's free.
    // If its not available, the allocation fails.
    //
    TlsfBlock *next = block->next;
    if (next == nullptr || !next->isFree()) {
        return OsStatusOutOfMemory;
    }

    //
    // Try and claim the extra size needed to grow the block, we only need to check
    // one block after this one as adjacent blocks are merged when freed.
    //
    size_t extraSize = size - block->size;
    if (next->size >= extraSize) {
        block->size = size;
        next->offset += extraSize;

        //
        // Try and merge this block with the adjacent block if we used all of its space.
        //
        if (next->size == extraSize) {
            KM_CHECK(next != mNullBlock, "Next block is null");
            takeBlockFromFreeList(next);
            mergeBlock(next, block);
            *result = TlsfAllocation(next);
        } else {
            //
            // Otherwise just resize the next block to account for the space we took.
            //
            if (next != mNullBlock) {
                resizeBlock(next, next->size - extraSize);
            } else {
                next->size -= extraSize;
            }
            *result = TlsfAllocation(block);
        }

        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}

OsStatus TlsfHeap::shrink(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    if (size == block->size) {
        *result = ptr;
        return OsStatusSuccess;
    }

    if (size > block->size) {
        return OsStatusInvalidInput;
    }

    TlsfAllocation lo, hi;
    if (OsStatus status = split(ptr, block->offset + size, &lo, &hi)) {
        return status;
    }

    free(hi);
    *result = lo;

    return OsStatusSuccess;
}

OsStatus TlsfHeap::resize(TlsfAllocation ptr, size_t size, TlsfAllocation *result) [[clang::allocating]] {
    if (size == 0) {
        return OsStatusInvalidInput;
    }

    TlsfBlock *block = ptr.getBlock();
    //
    // Early out if the size is the same as the current size.
    //
    if (size == block->size) {
        *result = ptr;
        return OsStatusSuccess;
    }

    if (size < block->size) {
        //
        // Shrink the block.
        //
        return shrink(ptr, size, result);
    } else {
        //
        // Expand the block.
        //
        return grow(ptr, size, result);
    }
}

km::TlsfAllocation TlsfHeap::allocBestFit(size_t align, size_t size) [[clang::allocating]] {
    TlsfAllocation result;
    size_t prevIndex = 0;
    TlsfBlock *prevListBlock = findFreeBlock(size, &prevIndex);
    while (prevListBlock != nullptr) {
        if (checkBlock(prevListBlock, prevIndex, size, align, &result)) {
            return result;
        }

        prevListBlock = prevListBlock->propNextFree();
    }

    if (checkBlock(mNullBlock, mFreeListCount, size, align, &result)) {
        return result;
    }

    size_t nextIndex = 0;
    size_t nextSize = detail::GetNextBlockSize(size);
    TlsfBlock *nextListBlock = findFreeBlock(nextSize, &nextIndex);
    while (nextListBlock != nullptr) {
        if (checkBlock(nextListBlock, nextIndex, size, align, &result)) {
            return result;
        }

        nextListBlock = nextListBlock->propNextFree();
    }

    return km::TlsfAllocation{};
}

km::TlsfAllocation TlsfHeap::aligned_alloc(size_t align, size_t size) {
    TlsfAllocation result = allocBestFit(align, size);
    if (result.isValid()) {
        mMallocCount += 1;
    }
    return result;
}

TlsfAllocation TlsfHeap::allocateWithHint(size_t align, size_t size, PhysicalAddressEx hint) [[clang::allocating]] {
    if (auto allocation = allocateAt(hint, size)) {
        return allocation;
    }

    //
    // TODO: We could be smarter here and try and find the closest block to the hint
    //
    return allocBestFit(align, size);
}

TlsfAllocation TlsfHeap::allocateAt(PhysicalAddressEx address, size_t size) [[clang::allocating]] {
    //
    // Check preconditions.
    //
    if (!address.isAlignedTo(alignof(std::max_align_t))) {
        return TlsfAllocation{};
    }

    if (size == 0) {
        return TlsfAllocation{};
    }

    MemoryRange range = MemoryRange::of(address.address, size);

    TlsfAllocation result;
    if (reserve(range, &result) != OsStatusSuccess) {
        return TlsfAllocation{};
    }

    return result;
}

void TlsfHeap::free(TlsfAllocation address) noexcept [[clang::nonallocating]] {
    KM_CHECK(address.isValid(), "Invalid address");

    TlsfBlock *block = address.getBlock();
    KM_CHECK(!block->isFree(), "Block is already free");

    TlsfBlock *prev = block->prev;
    TlsfBlock *next = block->next;

    if (prev != nullptr && prev->isFree()) {
        takeBlockFromFreeList(prev);
        mergeBlock(block, prev);
    }

    if (!next->isFree()) {
        releaseBlockToFreeList(block);
    } else if (next == mNullBlock) {
        mergeBlock(mNullBlock, block);
    } else {
        takeBlockFromFreeList(next);
        mergeBlock(next, block);
        releaseBlockToFreeList(next);
    }

    mFreeCount += 1;
}

void TlsfHeap::freeAddress(PhysicalAddress address) noexcept [[clang::nonallocating]] {
    TlsfAllocation allocation;
    if (OsStatus status = findAllocation(address, &allocation)) {
        MemLog.fatalf("Failed to find allocation for ", address, " (", status, ")");
        KM_PANIC("Failed to find allocation for address");
    }
    free(allocation);
}

OsStatus TlsfHeap::findAllocation(PhysicalAddress address, TlsfAllocation *result) noexcept [[clang::nonallocating]] {
    TlsfBlock *block = mNullBlock;
    while (block != nullptr) {
        if (block->offset == address) {
            *result = TlsfAllocation(block);
            return OsStatusSuccess;
        }

        block = block->prev;
    }

    return OsStatusNotFound;
}

void TlsfHeap::splitBlock(TlsfBlock *block, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi, TlsfBlock *newBlock) noexcept [[clang::nonallocating]] {
    size_t base = block->offset;
    size_t size = midpoint.address - base;
    size_t extraSize = block->size - size;

    KM_ASSERT(size > 0);
    KM_ASSERT(extraSize > 0);

    *newBlock = TlsfBlock {
        .offset = midpoint.address,
        .size = extraSize,
        .prev = block,
        .next = block->next,
    };
    block->size = size;
    block->next = newBlock;
    newBlock->next->prev = newBlock;
    newBlock->markTaken();

    *lo = TlsfAllocation(block);
    *hi = TlsfAllocation(newBlock);
}

OsStatus TlsfHeap::split(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    KM_CHECK(block != nullptr, "Block is null");
    KM_CHECK(block != mNullBlock, "Block is null");
    KM_CHECK(!block->isFree(), "Block is already free");

    PhysicalAddress base = block->offset;
    PhysicalAddress end = block->offset + block->size;

    if (midpoint < base || midpoint >= end) {
        return OsStatusInvalidInput;
    }

    size_t size = midpoint.address - base.address;

    if ((size == block->size) || (size > block->size) || (size == 0)) {
        return OsStatusInvalidInput;
    }

    TlsfBlock *newBlock = mBlockPool.construct();
    if (newBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    splitBlock(ptr.getBlock(), midpoint, lo, hi, newBlock);

    return OsStatusSuccess;
}

OsStatus TlsfHeap::splitv(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) [[clang::allocating]] {
    static_assert(sizeof(TlsfAllocation) == sizeof(TlsfBlock*), "We reuse the result buffer to store allocated blocks");

    MemoryRange range = ptr.range();

    //
    // There must be points to split.
    //
    if (points.empty()) {
        return OsStatusInvalidInput;
    }

    //
    // The results buffer must be large enough to hold all results.
    //
    if (points.size() >= (results.size() + 1)) {
        return OsStatusInvalidInput;
    }

    //
    // If we're building with hardening enabled, perform extra checks on the input.
    //
    if constexpr (kEnableSplitVectorHardening) {
        //
        // All points must be sorted in ascending order.
        //
        if (!std::ranges::is_sorted(points)) {
            return OsStatusInvalidInput;
        }

        //
        // There must be no duplicate elements.
        //
        if (std::ranges::adjacent_find(points) != std::ranges::end(points)) {
            return OsStatusInvalidInput;
        }

        //
        // All points must be within the range of the pointer.
        //
        for (PhysicalAddress point : points) {
            if (!range.contains(point) || range.front == point || range.back == point) {
                return OsStatusInvalidInput;
            }
        }
    }

    //
    // We borrow the results buffer to store the new blocks we need to allocate
    // for the splits.
    // TODO: is this kosher?
    //
    TlsfBlock **storage = reinterpret_cast<TlsfBlock**>(results.data());
    for (size_t i = 1; i < points.size() + 1; i++) {
        TlsfBlock *block = mBlockPool.construct();
        if (block == nullptr) {
            //
            // If we fail to allocate here we need to roll back all the previous allocations.
            //
            for (size_t j = i; j > 1; j--) {
                mBlockPool.destroy(storage[j - 1]);
            }

            return OsStatusOutOfMemory;
        }

        storage[i] = block;
    }

    TlsfBlock *next = storage[1];
    TlsfAllocation lo = ptr;
    TlsfAllocation hi;
    for (size_t i = 0; i < points.size() - 1; i++) {
        splitBlock(lo.getBlock(), points[i], &lo, &hi, next);
        next = storage[i + 2];
        results[i] = lo;
        lo = hi;
    }
    splitBlock(lo.getBlock(), points.back(), &lo, &hi, next);
    results[points.size() - 1] = lo;
    results[points.size()] = hi;

    return OsStatusSuccess;
}

OsStatus TlsfHeap::splitBlockForAllocationIfRequired(TlsfBlock *block, size_t alignedOffset) noexcept [[clang::allocating]] {
    if ((alignedOffset != block->offset) && block->prev == nullptr) {
        TlsfBlock *newBlock = mBlockPool.construct(TlsfBlock {
            .offset = block->offset,
            .size = alignedOffset - block->offset,
            .prev = nullptr,
            .next = block,
        });
        if (newBlock == nullptr) {
            return OsStatusOutOfMemory;
        }

        block->prev = newBlock;
        block->offset = alignedOffset;
        block->size -= newBlock->size;
        newBlock->markTaken();
        releaseBlockToFreeList(newBlock);
    }

    return OsStatusSuccess;
}

OsStatus TlsfHeap::reserve(MemoryRange range, TlsfAllocation *result) [[clang::allocating]] {
    compact();

    TlsfBlock *block = mNullBlock;
    while (block != nullptr) {
        km::MemoryRange blockRange { block->offset, block->offset + block->size };
        if (blockRange.contains(range) || blockRange == range) {
            if (block->isUsed()) {
                // The range is already in use
                return OsStatusNotAvailable;
            }

            if (OsStatus status = splitBlockForAllocationIfRequired(block, range.front.address)) {
                return status;
            }

            if (!reserveBlock(block, range.size(), range.front.address, detail::GetListIndex(block->size), result)) {
                return OsStatusOutOfMemory;
            }

            return OsStatusSuccess;
        }

        block = block->prev;
    }

    return OsStatusNotFound;
}

void TlsfHeap::detachBlock(TlsfBlock *block, size_t listIndex) noexcept [[clang::nonblocking]] {
    if (listIndex != mFreeListCount && block->prevFree != nullptr) {
        block->prevFree->propNextFree() = block->propNextFree();
        if (block->propNextFree() != nullptr) {
            block->propNextFree()->prevFree = block->prevFree;
        }
        block->prevFree = nullptr;
        block->propNextFree() = mFreeList[listIndex];
        mFreeList[listIndex] = block;
        if (block->propNextFree() != nullptr) {
            block->propNextFree()->prevFree = block;
        }
    }
}

bool TlsfHeap::checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, TlsfAllocation *result) [[clang::allocating]] {
    KM_ASSERT(block != nullptr);
    KM_ASSERT(block->isFree());

    size_t alignedOffset = sm::roundup(block->offset, align);
    if (block->size < (size + alignedOffset - block->offset)) {
        return false;
    }

    if (reserveBlock(block, size, alignedOffset, listIndex, result)) {
        return true;
    }

    return false;
}

bool TlsfHeap::reserveBlock(TlsfBlock *block, size_t size, size_t alignedOffset, size_t listIndex, TlsfAllocation *result) noexcept [[clang::allocating]] {
    KM_ASSERT(block != nullptr);

    size_t missingAlignment = alignedOffset - block->offset;
    BlockBuffer<2> buffer(&mBlockPool);

    //
    // Precompute the number of blocks that need to be created, in the case we cant allocate
    // the new blocks then we bail early to avoid complex rollback logic.
    //
    size_t innerSize = block->size;
    size_t requiredBlocks = 0;
    if (missingAlignment > 0) {
        if (!block->prev->isFree()) {
            requiredBlocks += 1;
        }

        innerSize -= missingAlignment;
    }

    if (!((innerSize == size) && (block != mNullBlock))) {
        requiredBlocks += 1;
    }

    if (buffer.reserve(requiredBlocks) != OsStatusSuccess) {
        return false;
    }

    detachBlock(block, listIndex);

    if (block != mNullBlock) {
        takeBlockFromFreeList(block);
    }

    if (missingAlignment > 0) {
        TlsfBlock *prev = block->prev;
        KM_ASSERT(prev != nullptr);

        if (prev->isFree()) {
            resizeBlock(prev, prev->size + missingAlignment);
        } else {
            TlsfBlock *newBlock = buffer.take(TlsfBlock {
                .offset = block->offset,
                .size = missingAlignment,
                .prev = prev,
                .next = block,
            });

            block->prev = newBlock;
            prev->next = newBlock;
            newBlock->markTaken();

            releaseBlockToFreeList(newBlock);
        }

        block->size -= missingAlignment;
        block->offset += missingAlignment;
    }

    if (block->size == size) {
        if (block == mNullBlock) {
            TlsfBlock *newNullBlock = buffer.take(TlsfBlock {
                .offset = block->offset + size,
                .size = 0,
                .prev = block,
                .next = nullptr,
            });

            mNullBlock = newNullBlock;
            block->next = mNullBlock;
            block->markTaken();
        }
    } else {
        KM_CHECK(block->size > size, "Block size is less than requested size");

        TlsfBlock *newBlock = buffer.take(TlsfBlock {
            .offset = block->offset + size,
            .size = block->size - size,
            .prev = block,
            .next = block->next,
        });

        block->next = newBlock;
        block->size = size;

        if (block == mNullBlock) {
            mNullBlock = newBlock;
            block->markTaken();
        } else {
            newBlock->next->prev = newBlock;
            newBlock->markTaken();
            releaseBlockToFreeList(newBlock);
        }
    }

    KM_CHECK(!block->isFree(), "Block is free");

    *result = TlsfAllocation(block);
    return true;
}

void TlsfHeap::takeBlockFromFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]] {
    KM_ASSERT(block != mNullBlock);
    KM_ASSERT(block->isFree());

    if (block->propNextFree() != nullptr) {
        block->propNextFree()->prevFree = block->prevFree;
    }

    if (block->prevFree != nullptr) {
        block->prevFree->propNextFree() = block->propNextFree();
    } else {
        uint8_t memoryClass = detail::SizeToMemoryClass(block->size);
        uint16_t secondIndex = detail::SizeToSecondIndex(block->size, memoryClass);
        size_t listIndex = detail::GetListIndex(memoryClass, secondIndex);

        mFreeList[listIndex] = block->propNextFree();
        if (block->propNextFree() == nullptr) {
            mInnerFreeMap[memoryClass] &= ~(1u << secondIndex);
            if (mInnerFreeMap[memoryClass] == 0) {
                mTopLevelFreeMap &= ~(1u << memoryClass);
            }
        }
    }

    block->markTaken();
}

void TlsfHeap::releaseBlockToFreeList(TlsfBlock *block) noexcept [[clang::nonblocking]] {
    KM_ASSERT(block != mNullBlock);
    KM_ASSERT(!block->isFree());

    uint8_t memoryClass = detail::SizeToMemoryClass(block->size);
    uint16_t secondIndex = detail::SizeToSecondIndex(block->size, memoryClass);
    size_t listIndex = detail::GetListIndex(memoryClass, secondIndex);

    block->markFree();
    block->propNextFree() = mFreeList[listIndex];
    mFreeList[listIndex] = block;
    if (block->propNextFree() != nullptr) {
        block->propNextFree()->prevFree = block;
    } else {
        mInnerFreeMap[memoryClass] |= (1u << secondIndex);
        mTopLevelFreeMap |= (1u << memoryClass);
    }
}

void TlsfHeap::mergeBlock(TlsfBlock *block, TlsfBlock *prev) noexcept [[clang::nonallocating]] {
    KM_ASSERT(block->prev == prev);
    KM_ASSERT(!prev->isFree());

    block->offset = prev->offset;
    block->size += prev->size;
    block->prev = prev->prev;
    if (block->prev != nullptr) {
        block->prev->next = block;
    }
    mBlockPool.destroy(prev);
}

void TlsfHeap::drainBlockList(detail::TlsfBlockList list) noexcept [[clang::nonallocating]] {
    while (TlsfBlock *block = list.drain()) {
        mBlockPool.release(block);
    }
}

OsStatus TlsfHeap::allocateSplitBlocks(detail::TlsfBlockList& list) [[clang::allocating]] {
    return allocateSplitVectorBlocks(1, list);
}

OsStatus TlsfHeap::allocateSplitVectorBlocks(size_t points, detail::TlsfBlockList& list) [[clang::allocating]] {
    detail::TlsfBlockList buffer;
    for (size_t i = 0; i < points; i++) {
        TlsfBlock *block = mBlockPool.construct();
        if (block == nullptr) {
            drainBlockList(std::move(buffer));
            return OsStatusOutOfMemory;
        }

        buffer.add(block);
    }

    list.append(std::move(buffer));
    return OsStatusSuccess;
}

void TlsfHeap::commitSplitBlock(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi, detail::TlsfBlockList& list) noexcept [[clang::nonallocating]] {
    TlsfBlock *block = list.next();
    splitBlock(ptr.getBlock(), midpoint, lo, hi, block);
}

void TlsfHeap::commitSplitVectorBlocks(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results, detail::TlsfBlockList& list) noexcept [[clang::nonallocating]] {
    TlsfBlock *next = list.next();
    TlsfAllocation lo = ptr;
    TlsfAllocation hi;
    for (size_t i = 0; i < points.size() - 1; i++) {
        splitBlock(lo.getBlock(), points[i], &lo, &hi, next);
        results[i] = lo;
        lo = hi;
        next = list.next();
    }
    splitBlock(lo.getBlock(), points.back(), &lo, &hi, next);
    results[points.size() - 1] = lo;
    results[points.size()] = hi;
}

TlsfHeapStats TlsfHeap::stats() const noexcept [[clang::nonallocating]] {
    size_t blockCount = 0;
    size_t freeMemory = 0;
    auto countBlockStats = [&](const TlsfBlock *block) {
        const TlsfBlock *it = block;
        while (it != nullptr) {
            blockCount += 1;
            if (it->isFree()) {
                freeMemory += it->size;
            }
            it = it->prev;
        }
    };

    countBlockStats(mNullBlock);

    size_t usedMemory = mSize - freeMemory;
    return TlsfHeapStats {
        .pool = mBlockPool.stats(),
        .freeListSize = mFreeListCount,
        .usedMemory = usedMemory - mReserved,
        .freeMemory = freeMemory,
        .blockCount = blockCount,
        .mallocCount = mMallocCount,
        .freeCount = mFreeCount,
    };
}

void TlsfHeap::reset() noexcept [[clang::nonallocating]] {
    TlsfBlock *block = mNullBlock;
    while (block != nullptr && block->prev != nullptr) {
        TlsfBlock *prev = block->prev;
        KM_ASSERT(prev->offset + prev->size == block->offset);
        block->offset = prev->offset;
        block->size += prev->size;
        block->prev = prev->prev;
        mBlockPool.destroy(prev);

        block = block->prev;
    }

    mMallocCount = 0;
    mFreeCount = 0;
}
