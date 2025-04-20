#include "memory/detail/heap.hpp"

using TlsfBlock = km::detail::TlsfBlock;
using TlsfHeap = km::detail::TlsfHeap;

static constexpr size_t GetMemoryClass(size_t size) {
    if (size > TlsfHeap::kSmallBufferSize) {
        return std::countl_zero(size) - TlsfHeap::kMemoryClassShift;
    }

    return 0;
}

static constexpr size_t GetSecondIndex(size_t size, size_t memoryClass) {
    if (memoryClass == 0) {
        return ((size - 1) / 8);
    } else {
        return ((size >> (memoryClass + TlsfHeap::kMemoryClassShift - TlsfHeap::kSecondLevelIndex)) ^ (1 << TlsfHeap::kSecondLevelIndex));
    }
}

static constexpr size_t GetFreeListSize(size_t memoryClass, size_t secondIndex) {
    return (memoryClass == 0 ? 0 : (memoryClass - 1) * (1 << TlsfHeap::kSecondLevelIndex) + secondIndex) + 1 + (1 << TlsfHeap::kSecondLevelIndex);
}

static constexpr size_t GetListIndex(size_t memoryClass, size_t secondIndex) {
    if (memoryClass == 0) {
        return secondIndex;
    }

    size_t index = (memoryClass - 1) * (1 << TlsfHeap::kSecondLevelIndex) + secondIndex;
    return index + (1 << TlsfHeap::kSecondLevelIndex);
}

static constexpr size_t GetListIndex(size_t size) {
    size_t memoryClass = GetMemoryClass(size);
    size_t secondIndex = GetSecondIndex(size, memoryClass);
    return GetListIndex(memoryClass, secondIndex);
}

TlsfBlock *TlsfHeap::findFreeBlock(size_t size, size_t *listIndex) {
    size_t memoryClass = GetMemoryClass(size);
    uint32_t innerFreeMap = mInnerFreeBitMap[memoryClass] & (~0u << GetSecondIndex(size, memoryClass));
    if (innerFreeMap == 0) {
        uint32_t freeMap = mIsFreeMap & (~0u << (memoryClass + 1));
        if (freeMap == 0) {
            return nullptr;
        }

        memoryClass = std::countr_zero(freeMap);
        innerFreeMap = mInnerFreeBitMap[memoryClass];
    }

    size_t index = GetListIndex(memoryClass, std::countr_zero(innerFreeMap));
    *listIndex = index;
    return mFreeList[index];
}

TlsfHeap::TlsfHeap(MemoryRange range) {
    size_t size = range.size();
    mNullBlock = mBlockPool.construct(TlsfBlock {
        .offset = 0,
        .size = size,
    });

    size_t memoryClass = GetMemoryClass(size);
    size_t secondIndex = GetSecondIndex(size, memoryClass);
    mFreeListSize = GetFreeListSize(memoryClass, secondIndex);
    mMemoryClassCount = memoryClass + 2;
    mFreeList = std::make_unique<BlockPtr[]>(mFreeListSize);
    std::uninitialized_fill_n(mInnerFreeBitMap, kMaxMemoryClass, 0);
}

TlsfHeap::~TlsfHeap() {
    mBlockPool.clear();
}

void TlsfHeap::validate() {

}

void *TlsfHeap::aligned_alloc(size_t align, size_t size) {
    size_t prevIndex = 0;
    TlsfBlock *block = findFreeBlock(size, &prevIndex);
    while (block != nullptr) {
        void *result = nullptr;
        if (checkBlock(block, prevIndex, size, align, &result)) {
            return result;
        }

        block = block->nextFree;
    }

    return nullptr;
}

void TlsfHeap::free(void *ptr) {

}

bool TlsfHeap::checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, void **result) {
    size_t alignedOffset = sm::roundup(block->offset, align);
    if (block->size < (size + alignedOffset - block->offset)) {
        return false;
    }

    if (listIndex != mFreeListSize && block->prevFree != nullptr) {
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

    *result = alloc(block, size, alignedOffset);
    return true;
}

void *TlsfHeap::alloc(TlsfBlock *block, size_t size, size_t alignedOffset) {
    if (block != mNullBlock) {
        removeFreeBlock(block);
    }

    size_t missingAlignment = alignedOffset - block->offset;
    if (missingAlignment > 0) {
        TlsfBlock *prevBlock = block->prev;
        KM_ASSERT(prevBlock != nullptr);

        if (prevBlock->isFree()) {
            size_t oldList = GetListIndex(prevBlock->size);
            prevBlock->size += missingAlignment;
            if (oldList != GetListIndex(prevBlock->size)) {
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
            block->prev = newBlock;
            prevBlock->next = newBlock;
            newBlock->retain();

            insertFreeBlock(newBlock);
        }

        block->size -= missingAlignment;
        block->offset += alignedOffset;
    }

    if (block->size == size) {
        if (block == mNullBlock) {
            mNullBlock = mBlockPool.construct(TlsfBlock {

            });
        }
    } else {

    }
}
