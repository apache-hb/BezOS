#include "memory/page_allocator.hpp"

using namespace km;

size_t km::detail::GetRangeBitmapSize(MemoryRange range) {
    // Round up to the nearest byte to ensure
    // each allocator has a bitmap start and
    // end that doesn't spill into other allocators.
    size_t pages = sm::roundup<size_t>(km::pages(range.size()), CHAR_BIT);
    return pages / CHAR_BIT;
}

size_t RegionBitmapAllocator::bitCount() const {
    return mRange.size() / x64::kPageSize;
}

bool RegionBitmapAllocator::test(size_t bit) const {
    return mBitmap[bit / CHAR_BIT] & (1 << (bit % CHAR_BIT));
}

void RegionBitmapAllocator::set(size_t bit) {
    mBitmap[bit / CHAR_BIT] |= 1 << (bit % CHAR_BIT);
}

void RegionBitmapAllocator::clear(size_t bit) {
    mBitmap[bit / CHAR_BIT] &= ~(1 << (bit % CHAR_BIT));
}

RegionBitmapAllocator::RegionBitmapAllocator(MemoryRange range, uint8_t *bitmap)
    : mRange(range)
    , mBitmap(bitmap)
{ }

PhysicalAddress RegionBitmapAllocator::alloc4k(size_t count) {
    // Find a sequence of free bits at least count long.
    size_t start = 0;
    size_t length = 0;
    for (size_t i = 0; i < bitCount(); i++) {
        if (test(i)) {
            start = i + 1;
            length = 0;
        } else {
            length++;
        }
    }

    // If we didn't find a sequence of free bits, return nullptr.
    if (length < count) {
        return nullptr;
    }

    // Mark the bits as used.
    for (size_t i = start; i < start + count; i++) {
        set(i);
    }

    return mRange.front + (start * x64::kPageSize);
}

void RegionBitmapAllocator::release(MemoryRange range) {
    range = intersection(range, mRange);
    if (range.isEmpty()) return;

    // Find the bit range that corresponds to the range.
    size_t start = (range.front - mRange.front) / x64::kPageSize;
    size_t end = (range.back - mRange.front) / x64::kPageSize;

    // Mark the bits as free.
    for (size_t i = start; i < end; i++) {
        clear(i);
    }
}

void RegionBitmapAllocator::markUsed(MemoryRange range) {
    range = intersection(range, mRange);
    if (range.isEmpty()) return;

    // Find the bit range that corresponds to the range.
    size_t start = (range.front - mRange.front) / x64::kPageSize;
    size_t end = (range.back - mRange.front) / x64::kPageSize;

    // Mark the bits as used.
    for (size_t i = start; i < end; i++) {
        set(i);
    }
}
