#include "log.hpp"
#include "memory/page_allocator.hpp"
#include "util/bits.hpp"

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
    return sm::BitsTestBit(mBitmap, sm::BitCount(bit));
}

void RegionBitmapAllocator::set(size_t bit) {
    sm::BitsSetBit(mBitmap, sm::BitCount(bit));
}

void RegionBitmapAllocator::clear(size_t bit) {
    sm::BitsClearBit(mBitmap, sm::BitCount(bit));
}

RegionBitmapAllocator::RegionBitmapAllocator(MemoryRange range, uint8_t *bitmap)
    : mRange(range)
    , mBitmap(bitmap)
{ }

PhysicalAddress RegionBitmapAllocator::alloc4k(size_t count) {
    // Find a sequence of free bits at least count long.
    sm::BitCount start = sm::BitsFindAndSetFreeRange(mBitmap, sm::BitCount(0), sm::BitCount(count), sm::BitCount(bitCount()));

    // If we didn't find a sequence of free bits, return nullptr.
    if (start.count >= bitCount()) {
        return nullptr;
    }

    return mRange.front + (start.count * x64::kPageSize);
}

void RegionBitmapAllocator::release(MemoryRange range) {
    range = intersection(range, mRange);
    if (range.isEmpty()) return;

    // Find the bit range that corresponds to the range.
    size_t start = (range.front - mRange.front) / x64::kPageSize;
    size_t end = (range.back - mRange.front) / x64::kPageSize;

    // Mark the bits as free.
    sm::BitsClearRange(mBitmap, sm::BitCount(start), sm::BitCount(end));
}

void RegionBitmapAllocator::markUsed(MemoryRange range) {
    range = intersection(range, mRange);
    if (range.isEmpty()) return;

    KmDebugMessage("[PMM] Marking range as used: ", range, "\n");

    // Find the bit range that corresponds to the range.
    size_t start = (range.front - mRange.front) / x64::kPageSize;
    size_t end = (range.back - mRange.front) / x64::kPageSize;

    // Mark the bits as used.
    sm::BitsSetRange(mBitmap, sm::BitCount(start), sm::BitCount(end));
}
