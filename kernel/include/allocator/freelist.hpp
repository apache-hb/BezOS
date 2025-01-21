#pragma once

#include "std/fixed_vector.hpp"

#include <cstdint>

namespace mem {
    struct FreeBlock {
        uint32_t offset;
        uint32_t length;

        bool isEmpty() const { return length == 0; }
    };

    class FreeVector {
        stdx::FixedVector<FreeBlock> mFreeBlocks;

    public:
        FreeVector(stdx::FixedVector<FreeBlock> blocks)
            : mFreeBlocks(blocks)
        { }

        FreeVector(FreeBlock *front, FreeBlock *back)
            : mFreeBlocks(front, back)
        { }

        FreeBlock findBlock(uint32_t size, uint32_t align = 1);
        void freeBlock(FreeBlock block);

        void compact();

        size_t count() const { return mFreeBlocks.count(); }
        size_t capacity() const { return mFreeBlocks.capacity(); }

        static constexpr size_t GetRequiredSize(size_t memory) {
            return GetRequiredBlocks(memory) * sizeof(FreeBlock);
        }

        static constexpr size_t GetRequiredBlocks(size_t memory) {
            // Worst case is when every other block is free
            return memory / 2;
        }
    };

    static_assert(FreeVector::GetRequiredSize(0x1000) == ((0x1000 / 2) * sizeof(FreeBlock)));
}
