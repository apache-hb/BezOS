#include "allocator/freelist.hpp"
#include <algorithm>

#include "panic.hpp"

using FreeVector = mem::FreeVector;

FreeVector::FreeBlock FreeVector::findBlock(uint32_t size, uint32_t align) {
    for (FreeBlock& block : mFreeBlocks) {
        if (block.length >= size) {
            uint32_t offset = block.offset;
            uint32_t padding = (align - (offset % align)) % align;
            if (block.length >= size + padding) {
                block.offset += size + padding;
                block.length -= size + padding;
                return { offset + padding, size };
            }
        }
    }

    return { UINT32_MAX, 0 };
}

void FreeVector::freeBlock(FreeBlock block) {
    if (!mFreeBlocks.add(block)) {
        // Emergency compaction to prevent forgetting about this block
        compact();

        bool ok = mFreeBlocks.add(block);
        KM_CHECK(ok, "Failed to add free block, free vector was not constructed with enough space for its memory region.");
    }
}

void FreeVector::compact() {
    std::sort(mFreeBlocks.begin(), mFreeBlocks.end(), [](const FreeBlock& a, const FreeBlock& b) {
        return a.offset < b.offset;
    });

    for (size_t i = 0; i < mFreeBlocks.count() - 1; ++i) {
        FreeBlock& a = mFreeBlocks[i];
        FreeBlock& b = mFreeBlocks[i + 1];

        if (a.offset + a.length == b.offset) {
            a.length += b.length;
            mFreeBlocks.erase(mFreeBlocks.begin() + i + 1);
            --i;
        }
    }
}
