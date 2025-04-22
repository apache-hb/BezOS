#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "std/static_vector.hpp"

#include <numeric>
#include <ranges>

using TlsfBlock = km::detail::TlsfBlock;
using TlsfHeap = km::TlsfHeap;
using TlsfHeapStats = km::TlsfHeapStats;
using TlsfAllocation = km::TlsfAllocation;

namespace {
    template<size_t N>
    struct BlockBuffer {
        km::PoolAllocator<TlsfBlock> *pool;
        stdx::StaticVector<TlsfBlock*, N> blocks;

        UTIL_NOCOPY(BlockBuffer);
        UTIL_NOMOVE(BlockBuffer);

        ~BlockBuffer() {
            for (TlsfBlock *block : blocks) {
                pool->destroy(block);
            }
        }

        BlockBuffer(km::PoolAllocator<TlsfBlock> *pool)
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
    : block(block)
{
    if (block != nullptr) {
        KM_CHECK(!block->isFree(), "Allocation was not created from a free block");
    }
}

TlsfBlock *TlsfHeap::findFreeBlock(size_t size, size_t *listIndex) noexcept [[clang::nonblocking]] {
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
    : mBlockPool(std::move(pool))
    , mNullBlock(nullBlock)
    , mFreeListCount(freeListCount)
    , mFreeList(std::move(freeList))
{
    mNullBlock->markFree();
    init();
    mSize = mNullBlock->size;
}

TlsfHeap::~TlsfHeap() {
    mBlockPool.clear();
}

OsStatus TlsfHeap::create(MemoryRange range, TlsfHeap *heap) [[clang::allocating]] {
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
        return OsStatusOutOfMemory;
    }

    *heap = TlsfHeap(std::move(pool), nullBlock, freeListCount, std::unique_ptr<BlockPtr[]>(freeList));
    return OsStatusSuccess;
}

OsStatus TlsfHeap::create(std::span<const MemoryRange> ranges, TlsfHeap *heap) [[clang::allocating]] {
    if (ranges.empty()) {
        return OsStatusInvalidInput;
    }

    if (ranges.front().isEmpty()) {
        return OsStatusInvalidInput;
    }

    size_t size = std::reduce(ranges.begin(), ranges.end(), 0zu, [](size_t acc, const MemoryRange& range) {
        return acc + range.size();
    });

    uint8_t memoryClass = detail::SizeToMemoryClass(size);
    uint16_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
    size_t freeListCount = detail::GetFreeListSize(memoryClass, secondIndex);

    PoolAllocator<TlsfBlock> pool;

    TlsfBlock *nullBlock = pool.construct(TlsfBlock {
        .offset = ranges.front().front.address,
        .size = ranges.front().size(),
    });
    if (nullBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    BlockPtr *freeList = new (std::nothrow) BlockPtr[freeListCount];
    if (freeList == nullptr) {
        return OsStatusOutOfMemory;
    }

    TlsfHeap result { std::move(pool), nullBlock, freeListCount, std::unique_ptr<BlockPtr[]>(freeList) };

    for (const MemoryRange& range : ranges | std::views::drop(1)) {
        if (OsStatus status = result.addPool(range)) {
            return status;
        }
    }

    *heap = std::move(result);
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
        .offset = 0,
        .size = 0,
        .prevFree = block,
    });

    block->next = sentinel;
    mSize += range.size();

    insertFreeBlock(block);
    return OsStatusSuccess;
}

void TlsfHeap::validate() {

}

km::TlsfCompactStats TlsfHeap::compact() {
    PoolCompactStats pool = mBlockPool.compact();
    return TlsfCompactStats { pool };
}

km::TlsfAllocation TlsfHeap::malloc(size_t size) [[clang::allocating]] {
    return aligned_alloc(8, size);
}

OsStatus TlsfHeap::grow(TlsfAllocation ptr, size_t size) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    if (size == block->size) {
        return OsStatusSuccess;
    }

    if (size < block->size) {
        return OsStatusInvalidInput;
    }

    // Expand the block
    TlsfBlock *next = block->next;
    if (next != nullptr && next->isFree()) {
        size_t newSize = block->size + next->size;
        size_t extraSize = size - block->size;
        if (newSize >= size) {
            block->size = size;
            next->size -= extraSize;
            next->offset = block->offset + size;

            return OsStatusSuccess;
        }
    }

    return OsStatusOutOfMemory;
}

OsStatus TlsfHeap::shrink(TlsfAllocation ptr, size_t size) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    if (size == block->size) {
        return OsStatusSuccess;
    }

    if (size > block->size) {
        return OsStatusInvalidInput;
    }

    size_t extraSize = block->size - size;

    // If the block ahead is free, we can migrate the end of our free space to it
    TlsfBlock *next = block->next;
    if (next != nullptr && next->isFree()) {
        size_t newSize = block->size + next->size;
        if (newSize >= size) {
            block->size = size;
            next->size += extraSize;
            next->offset = block->offset + size;

            return OsStatusSuccess;
        }
    }

    // Cut the block to create a smaller allocation
    TlsfBlock *newBlock = mBlockPool.construct(TlsfBlock {
        .offset = block->offset + size,
        .size = extraSize,
        .prev = block,
        .next = block->next,
    });
    if (newBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    block->size = size;
    block->next = newBlock;

    if (block == mNullBlock) {
        mNullBlock = newBlock;
        mNullBlock->markFree();
        mNullBlock->prevFree = nullptr;
        mNullBlock->nextFree = nullptr;
        block->markTaken();
    } else {
        newBlock->next->prev = newBlock;
        newBlock->markFree();
        insertFreeBlock(newBlock);
    }

    return OsStatusSuccess;
}

OsStatus TlsfHeap::resize(TlsfAllocation ptr, size_t size) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    if (size == block->size) {
        return OsStatusSuccess;
    }

    if (size < block->size) {
        // Shrink the block
        return shrink(ptr, size);
    } else {
        // Expand the block
        return grow(ptr, size);
    }
}

km::TlsfAllocation TlsfHeap::aligned_alloc(size_t align, size_t size) {
    TlsfAllocation result;
    size_t prevIndex = 0;
    TlsfBlock *prevListBlock = findFreeBlock(size, &prevIndex);
    while (prevListBlock != nullptr) {
        if (checkBlock(prevListBlock, prevIndex, size, align, &result)) {
            return result;
        }

        prevListBlock = prevListBlock->nextFree;
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

        nextListBlock = nextListBlock->nextFree;
    }

    return nullptr;
}

void TlsfHeap::free(TlsfAllocation address) noexcept [[clang::nonallocating]] {
    TlsfBlock *block = address.getBlock();
    KM_CHECK(!block->isFree(), "Block is already free");

    TlsfBlock *prev = block->prev;
    if (prev != nullptr && prev->isFree()) {
        removeFreeBlock(prev);
        mergeBlock(block, prev);
    }

    TlsfBlock *next = block->next;
    if (!next->isFree()) {
        insertFreeBlock(block);
    } else if (next == mNullBlock) {
        mergeBlock(mNullBlock, block);
    } else {
        removeFreeBlock(next);
        mergeBlock(next, block);
        insertFreeBlock(next);
    }
}

OsStatus TlsfHeap::split(TlsfAllocation ptr, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi) [[clang::allocating]] {
    TlsfBlock *block = ptr.getBlock();
    PhysicalAddress base = block->offset;
    PhysicalAddress end = block->offset + block->size;

    if (midpoint < base || midpoint >= end) {
        return OsStatusInvalidInput;
    }

    size_t size = midpoint.address - base.address;

    if (size == block->size) {
        return OsStatusInvalidInput;
    }

    if (size > block->size) {
        return OsStatusInvalidInput;
    }

    size_t extraSize = block->size - size;

    // Cut the block to create a smaller allocation
    TlsfBlock *newBlock = mBlockPool.construct(TlsfBlock {
        .offset = midpoint.address,
        .size = extraSize,
        .prev = block,
        .next = block->next,
    });
    if (newBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    block->size = size;
    block->next = newBlock;

    newBlock->markTaken();

    *lo = TlsfAllocation(block);
    *hi = TlsfAllocation(newBlock);

    return OsStatusSuccess;
}

km::PhysicalAddress TlsfHeap::addressOf(TlsfAllocation ptr) const noexcept [[clang::nonblocking]] {
    if (ptr.isNull()) {
        return PhysicalAddress();
    }

    TlsfBlock *block = ptr.getBlock();
    return PhysicalAddress(block->offset);
}

void TlsfHeap::detachBlock(TlsfBlock *block, size_t listIndex) noexcept [[clang::nonblocking]] {
    if (listIndex != mFreeListCount && block->prevFree != nullptr) {
        block->prevFree->nextFree = block->nextFree;
        if (block->nextFree != nullptr) {
            block->nextFree->prevFree = block->prevFree;
        }
        block->prevFree = nullptr;
        block->nextFree = mFreeList[listIndex];
        mFreeList[listIndex] = block;
        if (block->nextFree != nullptr) {
            block->nextFree->prevFree = block;
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
    size_t missingAlignment = alignedOffset - block->offset;
    BlockBuffer<2> buffer(&mBlockPool);

    // Precompute the number of blocks that need to be created, in the case we cant allocate
    // the new blocks then we bail early to avoid complex rollback logic.
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
        removeFreeBlock(block);
    }

    if (missingAlignment > 0) {
        TlsfBlock *prev = block->prev;
        KM_ASSERT(prev != nullptr);

        if (prev->isFree()) {
            size_t oldList = detail::GetListIndex(prev->size);
            if (oldList != detail::GetListIndex(prev->size + missingAlignment)) {
                removeFreeBlock(prev);
                prev->size += missingAlignment;
                insertFreeBlock(prev);
            }
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

            insertFreeBlock(newBlock);
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
            mNullBlock->markFree();
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
            mNullBlock->markFree();
            mNullBlock->prevFree = nullptr;
            mNullBlock->nextFree = nullptr;
            block->markTaken();
        } else {
            newBlock->next->prev = newBlock;
            newBlock->markFree();
            insertFreeBlock(newBlock);
        }
    }

    *result = TlsfAllocation(block);
    return true;
}

void TlsfHeap::removeFreeBlock(TlsfBlock *block) noexcept [[clang::nonblocking]] {
    KM_ASSERT(block != mNullBlock);
    KM_ASSERT(block->isFree());

    if (block->nextFree != nullptr) {
        block->nextFree->prevFree = block->prevFree;
    }

    if (block->prevFree != nullptr) {
        block->prevFree->nextFree = block->nextFree;
    } else {
        uint8_t memoryClass = detail::SizeToMemoryClass(block->size);
        uint16_t secondIndex = detail::SizeToSecondIndex(block->size, memoryClass);
        size_t listIndex = detail::GetListIndex(memoryClass, secondIndex);

        mFreeList[listIndex] = block->nextFree;
        if (block->nextFree == nullptr) {
            mInnerFreeMap[memoryClass] &= ~(1u << secondIndex);
            if (mInnerFreeMap[memoryClass] == 0) {
                mTopLevelFreeMap &= ~(1u << memoryClass);
            }
        }
    }

    block->markTaken();
}

void TlsfHeap::insertFreeBlock(TlsfBlock *block) noexcept [[clang::nonblocking]] {
    KM_ASSERT(block != mNullBlock);

    uint8_t memoryClass = detail::SizeToMemoryClass(block->size);
    uint16_t secondIndex = detail::SizeToSecondIndex(block->size, memoryClass);
    size_t listIndex = detail::GetListIndex(memoryClass, secondIndex);

    block->prevFree = nullptr;
    block->nextFree = mFreeList[listIndex];
    mFreeList[listIndex] = block;
    if (block->nextFree != nullptr) {
        block->nextFree->prevFree = block;
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

TlsfHeapStats TlsfHeap::stats() noexcept [[clang::nonallocating]] {
    size_t blockCount = 0;
    size_t freeMemory = 0;
    auto countBlockStats = [&](const TlsfBlock *block) {
        const TlsfBlock *it = block;
        while (it != nullptr) {
            blockCount += 1;
            if (it->isFree()) {
                freeMemory += it->size;
            }
            it = it->next;
        }
    };

    countBlockStats(mNullBlock);
    for (size_t i = 0; i < mFreeListCount; i++) {
        countBlockStats(mFreeList[i]);
    }

    size_t usedMemory = mSize - freeMemory;
    return TlsfHeapStats {
        .pool = mBlockPool.stats(),
        .freeListSize = mFreeListCount,
        .usedMemory = usedMemory,
        .freeMemory = freeMemory,
        .blockCount = blockCount,
    };
}
