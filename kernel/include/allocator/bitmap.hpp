#pragma once

#include "allocator/allocator.hpp"
#include "util/util.hpp"

#include <limits.h>
#include <span>

namespace mem {
    class BitmapAllocator : public mem::AllocatorMixin<BitmapAllocator> {
        void *mFront;
        void *mBack;

        size_t mUnit;

        void init(void *front, void *back, size_t unit) {
            mFront = front;
            mBack = back;
            mUnit = unit;

            auto bits = bitmap();
            std::uninitialized_fill(bits.begin(), bits.end(), 0);
        }

        size_t bitmapSize() const {
            return (size() / mUnit) / CHAR_BIT;
        }

        std::span<uint8_t> bitmap() {
            return { (uint8_t*)mFront, bitmapSize() };
        }

        size_t allocateBitmapSpace(size_t count, size_t align) {
            size_t start = 0;
            size_t length = 0;

            std::span<uint8_t> bits = bitmap();

            for (size_t i = 0; i < (bits.size() * CHAR_BIT); i++) {
                if (length == 0 && i % align != 0) {
                    continue;
                }

                if (bits[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
                    start = i + 1;
                    length = 0;
                } else {
                    length++;
                }
            }

            if (length < count) {
                return SIZE_MAX;
            }

            for (size_t i = start; i < start + count; i++) {
                bits[i / CHAR_BIT] |= 1 << (i % CHAR_BIT);
            }

            return start;
        }

        void *memoryFront() {
            return (uint8_t*)mFront + sm::roundup(bitmapSize(), mUnit);
        }

        void *memoryBack() {
            return mBack;
        }

    public:
        /// @brief Construct a new bitmap allocator.
        ///
        /// @pre @a front and @a back must be aligned to @a unit.
        /// @pre @a unit must be a power of 2.
        /// @pre @a front must be less than @a back.
        ///
        /// @param front The front of the memory region.
        /// @param back The back of the memory region.
        /// @param unit The unit size of the allocator.
        BitmapAllocator(void *front, void *back, size_t unit = 8) {
            init(front, back, unit);
        }

        size_t size() const { return (uintptr_t)mBack - (uintptr_t)mFront; }

        /// @brief Allocate a block of memory.
        ///
        /// @pre @a align must be a multiple of @a unit.
        ///
        /// @param size The size of the block.
        /// @param align The alignment of the block.
        ///
        /// @return The block of memory or @c nullptr if the allocation failed.
        void *allocate(size_t size, size_t align = alignof(std::max_align_t)) {
            if (align % mUnit != 0) {
                return nullptr;
            }

            size_t total = sm::roundup(size, mUnit);

            size_t front = allocateBitmapSpace(total / mUnit, align / mUnit);

            if (front == SIZE_MAX) {
                return nullptr;
            }

            return (uint8_t*)memoryFront() + (front * mUnit);
        }

        /// @brief Deallocate a block of memory.
        ///
        /// @pre @a ptr must have been allocated by this allocator.
        /// @pre @a size must be the same as the size passed to @ref allocate.
        ///
        /// @param ptr The block of memory.
        /// @param size The size of the block.
        void deallocate(void *ptr, size_t size) {
            if (ptr < memoryFront() || ptr >= memoryBack()) {
                return;
            }

            size_t offset = (uintptr_t)ptr - (uintptr_t)memoryFront();
            size_t front = offset / mUnit;

            std::span<uint8_t> bits = bitmap();

            for (size_t i = front; i < front + (size / mUnit); i++) {
                bits[i / CHAR_BIT] &= ~(1 << (i % CHAR_BIT));
            }
        }
    };
}
