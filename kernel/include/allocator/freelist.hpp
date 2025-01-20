#pragma once

#include "std/fixed_vector.hpp"

#include <cstdint>

namespace mem {
    class FreeVector {
    public:
        struct FreeBlock {
            uint32_t offset;
            uint32_t length;

            bool isEmpty() const { return length == 0; }
        };

    private:
        stdx::FixedVector<FreeBlock> mFreeBlocks;

    public:
        FreeVector(FreeBlock *front, FreeBlock *back)
            : mFreeBlocks(front, back)
        { }

        FreeBlock findBlock(uint32_t size, uint32_t align = 1);
        void freeBlock(FreeBlock block);

        void compact();

        size_t count() const { return mFreeBlocks.count(); }
        size_t capacity() const { return mFreeBlocks.capacity(); }

        static constexpr size_t GetRequiredSize(size_t memory) {
            // Worst case is when every other block is free
            size_t blocks = memory / 2;
            return blocks * sizeof(FreeBlock);
        }
    };

    static_assert(FreeVector::GetRequiredSize(0x1000) == ((0x1000 / 2) * sizeof(FreeVector::FreeBlock)));
}
