#include "allocator/bitmap.hpp"

#include "kernel.hpp"

#include "util/util.hpp"

#include <limits.h>

void mem::BitmapAllocator::init(void *front, void *back, size_t unit) {
    mFront = front;
    mBack = back;
    mUnit = unit;

    auto bits = bitmap();
    std::uninitialized_fill(bits.begin(), bits.end(), 0);
}

size_t mem::BitmapAllocator::bitmapSize() const {
    return (size() / mUnit) / CHAR_BIT;
}

std::span<uint8_t> mem::BitmapAllocator::bitmap() {
    return { (uint8_t*)mFront, bitmapSize() };
}

size_t mem::BitmapAllocator::allocateBitmapSpace(size_t count, size_t align) {
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

void *mem::BitmapAllocator::memoryFront() {
    return (uint8_t*)mFront + sm::roundup(bitmapSize(), mUnit);
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

    if (front == SIZE_MAX) {
        return nullptr;
    }

    return (uint8_t*)memoryFront() + (front * mUnit);
}

void mem::BitmapAllocator::deallocate(void *ptr, size_t size) {
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
