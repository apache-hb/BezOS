#pragma once

#include "memory/detail/pool.hpp"

namespace km::detail {
    struct TlsfBlock {
        size_t offset;
        size_t size;
        TlsfBlock *prev;
        TlsfBlock *next;
        TlsfBlock *nextFree;
        TlsfBlock *prevFree;

        bool isFree() const {
            return (prevFree != this);
        }

        void retain() {
            prevFree = this;
        }

        void release() {
            prevFree = nullptr;
        }
    };

    class TlsfHeap {
    public:
        static constexpr size_t kSmallBufferSize = 0x100;
        static constexpr size_t kMemoryClassShift = 7;
        static constexpr size_t kSecondLevelIndex = 5;
        static constexpr size_t kMaxMemoryClass = 64 - kMemoryClassShift;

        using BlockPtr = TlsfBlock*;

    private:
        PoolAllocator<TlsfBlock> mBlockPool;
        size_t mFreeListSize;
        std::unique_ptr<BlockPtr[]> mFreeList;
        TlsfBlock *mNullBlock;

        size_t mMemoryClassCount;
        uint32_t mInnerFreeBitMap[kMaxMemoryClass];
        uint32_t mIsFreeMap;

        TlsfBlock *findFreeBlock(size_t size, size_t *listIndex);

        bool checkBlock(TlsfBlock *block, size_t listIndex, size_t size, size_t align, void **result);

        void *alloc(TlsfBlock *block, size_t size, size_t alignedOffset);

        void removeFreeBlock(TlsfBlock *block);
        void insertFreeBlock(TlsfBlock *block);

    public:
        UTIL_NOCOPY(TlsfHeap);
        UTIL_NOMOVE(TlsfHeap);

        TlsfHeap(MemoryRange range);
        ~TlsfHeap();

        void validate();

        void *malloc(size_t size);
        void *realloc(void *ptr, size_t size);
        void *aligned_alloc(size_t align, size_t size);
        void free(void *ptr);
    };
}
