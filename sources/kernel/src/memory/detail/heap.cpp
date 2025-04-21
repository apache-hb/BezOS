#include "memory/detail/heap.hpp"
#include "std/static_vector.hpp"

using TlsfBlock = km::TlsfBlock;
using TlsfHeap = km::TlsfHeap;
using TlsfHeapStats = km::TlsfHeapStats;

TlsfBlock *TlsfHeap::findFreeBlock(size_t size, size_t *listIndex) noexcept [[clang::nonallocating]] {
    KM_CHECK(size > 0, "size must be greater than 0");

    uint8_t memoryClass = detail::SizeToMemoryClass(size);
    uint16_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
    uint32_t innerFreeMap = mInnerFreeBitMap[memoryClass] & (~0u << secondIndex);
    if (innerFreeMap == 0) {
        uint32_t freeMap = mTopLevelFreeMap & (~0u << (memoryClass + 1));
        if (freeMap == 0) {
            return nullptr;
        }

        memoryClass = detail::BitScanTrailing(freeMap);
        innerFreeMap = mInnerFreeBitMap[memoryClass];
        KM_ASSERT(innerFreeMap != 0);
    }

    size_t index = detail::GetListIndex(memoryClass, detail::BitScanTrailing(innerFreeMap));
    *listIndex = index;
    return mFreeList[index];
}

TlsfHeap::TlsfHeap(MemoryRange range, PoolAllocator<TlsfBlock>&& pool, TlsfBlock *nullBlock, size_t freeListCount, std::unique_ptr<BlockPtr[]> freeList, size_t memoryClassCount)
    : mRange(range)
    , mBlockPool(std::move(pool))
    , mNullBlock(nullBlock)
    , mFreeListCount(freeListCount)
    , mFreeList(std::move(freeList))
    , mMemoryClassCount(memoryClassCount)
{
    mNullBlock->markFree();
    mTopLevelFreeMap = 0;
    std::uninitialized_fill_n(mFreeList.get(), freeListCount, nullptr);
    std::uninitialized_fill(std::begin(mInnerFreeBitMap), std::end(mInnerFreeBitMap), 0);
}

TlsfHeap::~TlsfHeap() {
    mBlockPool.clear();
}

OsStatus TlsfHeap::create(MemoryRange range, TlsfHeap *heap) [[clang::allocating]] {
    size_t size = range.size();
    uint8_t memoryClass = detail::SizeToMemoryClass(size);
    uint16_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
    size_t freeListCount = detail::GetFreeListSize(memoryClass, secondIndex);

    size_t memoryClassCount = memoryClass + 2;

    PoolAllocator<TlsfBlock> pool;
    TlsfBlock *nullBlock = pool.construct(TlsfBlock {
        .offset = 0,
        .size = range.size(),
    });
    if (nullBlock == nullptr) {
        return OsStatusOutOfMemory;
    }

    BlockPtr *freeList = new (std::nothrow) BlockPtr[freeListCount];
    if (freeList == nullptr) {
        return OsStatusOutOfMemory;
    }

    *heap = TlsfHeap(range, std::move(pool), nullBlock, freeListCount, std::unique_ptr<BlockPtr[]>(freeList), memoryClassCount);
    return OsStatusSuccess;
}

void TlsfHeap::validate() {

}

km::TlsfCompactStats TlsfHeap::compact() {
    PoolCompactStats pool = mBlockPool.compact();
    return TlsfCompactStats { pool };
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

km::PhysicalAddress TlsfHeap::addressOf(TlsfAllocation ptr) const noexcept [[clang::nonblocking]] {
    if (ptr.isNull()) {
        return PhysicalAddress();
    }

    TlsfBlock *block = ptr.getBlock();
    return PhysicalAddress(block->offset + mRange.front.address);
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
    stdx::StaticVector<TlsfBlock*, 2> blocks;

    auto releaseBlocks = [&] {
        for (TlsfBlock *b : blocks) {
            mBlockPool.destroy(b);
        }
    };

    auto getBlock = [&](TlsfBlock init) {
        KM_CHECK(!blocks.isEmpty(), "Temporary block vector is empty");
        TlsfBlock *newBlock = blocks.back();
        blocks.pop();
        *newBlock = init;
        return newBlock;
    };

    // Precompute the number of blocks that need to be created, in the case we cant allocate
    // the new blocks then we bail early to avoid complex rollback logic.
    size_t innerSize = block->size;
    if (missingAlignment > 0) {
        if (!block->prev->isFree()) {
            if (TlsfBlock *newBlock = mBlockPool.construct()) {
                blocks.add(newBlock);
            } else {
                return false;
            }
        }

        innerSize -= missingAlignment;
    }

    if (!((innerSize == size) && (block != mNullBlock))) {
        if (TlsfBlock *newBlock = mBlockPool.construct()) {
            blocks.add(newBlock);
        } else {
            releaseBlocks();
            return false;
        }
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
            TlsfBlock *newBlock = getBlock(TlsfBlock {
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
            TlsfBlock *newNullBlock = getBlock(TlsfBlock {
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

        TlsfBlock *newBlock = getBlock(TlsfBlock {
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
            mInnerFreeBitMap[memoryClass] &= ~(1u << secondIndex);
            if (mInnerFreeBitMap[memoryClass] == 0) {
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
        mInnerFreeBitMap[memoryClass] |= (1u << secondIndex);
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
    for (size_t i = 0; i < mFreeListCount; i++) {
        if (mFreeList[i] != nullptr) {
            BlockPtr block = mFreeList[i];
            while (block != nullptr) {
                blockCount += 1;
                block = block->next;
            }
        }
    }

    return TlsfHeapStats {
        .pool = mBlockPool.stats(),
        .freeListSize = mFreeListCount,
        .blockCount = blockCount,
        .controlMemory = (sizeof(BlockPtr) * mFreeListCount) + (sizeof(TlsfBlock) * blockCount),
    };
}
