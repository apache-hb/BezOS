#pragma once

#include "allocator/allocator.hpp"

#include <limits.h>
#include <span>

namespace mem {
    class BitmapAllocator final : public mem::IAllocator {
        void *mFront;
        void *mBack;

        size_t mUnit;

        void init(void *front, void *back, size_t unit);

        size_t bitmapWordCount() const;
        size_t bitmapBitCount() const;

        std::span<uint8_t> bitmap();

        size_t allocateBitmapSpace(size_t count, size_t align);

        void *memoryFront();

        void *memoryBack();

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
        BitmapAllocator(void *front, void *back, size_t unit = alignof(std::max_align_t));

        size_t size() const;

        /// @brief Allocate a block of memory.
        ///
        /// @param size The size of the block.
        /// @param align The alignment of the block.
        ///
        /// @return The block of memory or @c nullptr if the allocation failed.
        void *allocate(size_t size, size_t align = alignof(std::max_align_t)) override;

        /// @brief Deallocate a block of memory.
        ///
        /// @pre @a ptr must have been allocated by this allocator.
        /// @pre @a size must be the same as the size passed to @ref allocate.
        ///
        /// @param ptr The block of memory.
        /// @param size The size of the block.
        void deallocate(void *ptr, size_t size) override;
    };
}
