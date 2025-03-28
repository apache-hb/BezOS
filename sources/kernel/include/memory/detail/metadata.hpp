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
    };

    class TlsfMetadata {
        PoolAllocator<TlsfBlock> mBlockPool;
        size_t mFreeListSize;
        std::unique_ptr<TlsfBlock[]> mFreeList;
        TlsfBlock *mNullBlock;

        size_t mMemoryClassCount;

    public:
        UTIL_NOCOPY(TlsfMetadata);
        UTIL_NOMOVE(TlsfMetadata);

        TlsfMetadata(MemoryRange range);
        ~TlsfMetadata();

        void *malloc(size_t size);
        void *realloc(void *ptr, size_t size);
        void *aligned_alloc(size_t align, size_t size);
        void free(void *ptr);
    };
}
