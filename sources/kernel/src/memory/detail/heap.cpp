#include "memory/detail/heap.hpp"

using TlsfBlock = km::TlsfBlock;
using TlsfHeap = km::TlsfHeap;
using TlsfHeapStats = km::TlsfHeapStats;

TlsfBlock *TlsfHeap::findFreeBlock(size_t size, size_t *listIndex) noexcept [[clang::nonallocating]] {
    size_t memoryClass = detail::SizeToMemoryClass(size);
    uint32_t innerFreeMap = mInnerFreeBitMap[memoryClass] & (~0u << detail::SizeToSecondIndex(size, memoryClass));
    if (innerFreeMap == 0) {
        uint32_t freeMap = mIsFreeMap & (~0u << (memoryClass + 1));
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
    mIsFreeMap = 0;
    std::uninitialized_fill_n(mFreeList.get(), freeListCount, nullptr);
    std::uninitialized_fill(std::begin(mInnerFreeBitMap), std::end(mInnerFreeBitMap), 0);
}

TlsfHeap::~TlsfHeap() {
    mBlockPool.clear();
}

OsStatus TlsfHeap::create(MemoryRange range, TlsfHeap *heap) {
    size_t size = range.size();
    size_t memoryClass = detail::SizeToMemoryClass(size);
    size_t secondIndex = detail::SizeToSecondIndex(size, memoryClass);
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

void TlsfHeap::compact() {

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

bool TlsfHeap::checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, TlsfAllocation *result) {
    KM_ASSERT(block->isFree());

    size_t alignedOffset = sm::roundup(block->offset, align);
    if (block->size < (size + alignedOffset - block->offset)) {
        return false;
    }

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

    if (reserveBlock(block, size, alignedOffset, result)) {
        return true;
    }

    return false;
}

bool TlsfHeap::reserveBlock(TlsfBlock *block, size_t size, size_t alignedOffset, TlsfAllocation *result) noexcept [[clang::allocating]] {
    if (block != mNullBlock) {
        removeFreeBlock(block);
    }

    size_t missingAlignment = alignedOffset - block->offset;
    if (missingAlignment > 0) {
        TlsfBlock *prevBlock = block->prev;
        KM_ASSERT(prevBlock != nullptr);

        if (prevBlock->isFree()) {
            size_t oldList = detail::GetListIndex(prevBlock->size);
            prevBlock->size += missingAlignment;
            if (oldList != detail::GetListIndex(prevBlock->size)) {
                prevBlock->size -= missingAlignment;
                removeFreeBlock(prevBlock);
                prevBlock->size += missingAlignment;
                insertFreeBlock(prevBlock);
            }
        } else {
            TlsfBlock *newBlock = mBlockPool.construct(TlsfBlock {
                .offset = block->offset,
                .size = missingAlignment,
                .prev = prevBlock,
                .next = block,
            });
            if (newBlock == nullptr) {
                return false;
            }

            block->prev = newBlock;
            prevBlock->next = newBlock;
            newBlock->markTaken();

            insertFreeBlock(newBlock);
        }

        block->size -= missingAlignment;
        block->offset += alignedOffset;
    }

    if (block->size == size) {
        if (block == mNullBlock) {
            TlsfBlock *newNullBlock = mBlockPool.construct(TlsfBlock {
                .offset = block->offset + size,
                .size = 0,
                .prev = block,
                .next = nullptr,
            });
            if (newNullBlock == nullptr) {
                return false;
            }

            mNullBlock = newNullBlock;
            mNullBlock->markFree();
            block->next = mNullBlock;
            block->markTaken();
        }
    } else {
        KM_CHECK(block->size > size, "Block size is less than requested size");

        TlsfBlock *newBlock = mBlockPool.construct(TlsfBlock {
            .offset = block->offset + size,
            .size = block->size - size,
            .prev = block,
            .next = block->next,
        });
        if (newBlock == nullptr) {
            return false;
        }

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
        size_t memoryClass = detail::SizeToMemoryClass(block->size);
        size_t secondIndex = detail::SizeToSecondIndex(block->size, memoryClass);
        size_t listIndex = detail::GetListIndex(memoryClass, secondIndex);
        mFreeList[listIndex] = block->nextFree;
        if (block->nextFree == nullptr) {
            mInnerFreeBitMap[memoryClass] &= ~(1u << secondIndex);
            if (mInnerFreeBitMap[memoryClass] == 0) {
                mIsFreeMap &= ~(1u << memoryClass);
            }
        }
    }

    block->markTaken();
}

void TlsfHeap::insertFreeBlock(TlsfBlock *block) noexcept [[clang::nonblocking]] {
    KM_ASSERT(block != mNullBlock);
    KM_ASSERT(!block->isFree());

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
        mIsFreeMap |= (1u << memoryClass);
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

TlsfHeapStats TlsfHeap::stats() {
    return TlsfHeapStats {
        .pool = mBlockPool.stats(),
    };
}
