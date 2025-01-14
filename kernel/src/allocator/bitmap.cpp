#include "allocator/bitmap.hpp"

#include "log.hpp"
#include "util/util.hpp"

#include <limits.h>

void mem::BitmapAllocator::init(void *front, void *back, size_t unit) {
    mFront = front;
    mBack = back;
    mUnit = unit;

    auto bits = bitmap();
    std::uninitialized_fill(bits.begin(), bits.end(), 0);
}

size_t mem::BitmapAllocator::bitmapBitCount() const {
    return size() / mUnit;
}

size_t mem::BitmapAllocator::bitmapWordCount() const {
    return bitmapBitCount() / CHAR_BIT;
}

std::span<uint8_t> mem::BitmapAllocator::bitmap() {
    return { (uint8_t*)mFront, bitmapWordCount() };
}

static void SetBitRange(std::span<uint8_t> bits, size_t start, size_t count) {
    for (size_t i = start; i < start + count; i++) {
        bits[i / CHAR_BIT] |= 1 << (i % CHAR_BIT);
    }
}

static size_t ScanBitsForwards(std::span<uint8_t> bits, size_t count, size_t align) {
    size_t start = 0;
    size_t length = 0;

    for (size_t i = 0; i < (bits.size_bytes() * CHAR_BIT); i++) {
        if (length == 0 && i % align != 0) {
            continue;
        }

        if (bits[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
            start = i + 1;
            length = 0;
        } else {
            length++;
        }

        if (length >= count) {
            break;
        }
    }

    if (length < count) {
        return SIZE_MAX;
    }

    return start;
}

size_t mem::BitmapAllocator::allocateBitmapSpace(size_t count, size_t align) {
    auto bits = bitmap();

    size_t start = ScanBitsForwards(bits, count, align);
    if (start == SIZE_MAX) {
        KmDebugMessage("Failed to allocate ", count, " bits\n", km::HexDump(std::span<const uint8_t>(bits)), "\n");
        return SIZE_MAX;
    }

    SetBitRange(bits, start, count);

    return start;
}

void *mem::BitmapAllocator::memoryFront() {
    return (uint8_t*)mFront + sm::roundup(bitmapWordCount(), mUnit);
}

void *mem::BitmapAllocator::memoryBack() {
    return mBack;
}

mem::BitmapAllocator::BitmapAllocator(void *front, void *back, size_t unit) {
    init(front, back, unit);
}

size_t mem::BitmapAllocator::size() const { return (uintptr_t)mBack - (uintptr_t)mFront; }

void *mem::BitmapAllocator::allocate(size_t size, size_t align) {
    align = sm::roundup(align, mUnit);
    size = sm::roundup(size, mUnit);

    size_t front = allocateBitmapSpace(size / mUnit, align / mUnit);

    int pressure = 0;
    // count all set bits
    for (size_t i = 0; i < bitmapWordCount(); i++) {
        for (size_t j = 0; j < CHAR_BIT; j++) {
            if (bitmap()[i] & (1 << j)) {
                pressure++;
            }
        }
    }

    if (front == SIZE_MAX) {
        KmDebugMessage("Allocating ", size, " bytes with ", align, " alignment = ", (void*)nullptr, ", pressure = ", pressure, "/", bitmapWordCount() * 8,  "\n");
        return nullptr;
    }

    void *ptr = (uint8_t*)memoryFront() + (front * mUnit);

    return ptr;
}

void mem::BitmapAllocator::deallocate(void *ptr, size_t size) {
    if (ptr < memoryFront() || ptr >= memoryBack()) {
        return;
    }

    size = sm::roundup(size, mUnit);

    size_t offset = (uintptr_t)ptr - (uintptr_t)memoryFront();
    size_t front = offset / mUnit;

    std::span<uint8_t> bits = bitmap();

    for (size_t i = front; i < front + (size / mUnit); i++) {
        bits[i / CHAR_BIT] &= ~(1 << (i % CHAR_BIT));
    }
}
