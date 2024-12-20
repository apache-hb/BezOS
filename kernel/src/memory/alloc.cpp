#include "memory/virtual.hpp"

#include "kernel.hpp"

#include <limits.h>
#include <string.h>

using namespace km;

extern "C" {
    extern char __text_start[]; // always aligned to 0x1000
    extern char __text_end[];

    extern char __rodata_start[]; // always aligned to 0x1000
    extern char __rodata_end[];

    extern char __data_start[]; // always aligned to 0x1000
    extern char __data_end[];

    extern char __bss_start[]; // always aligned to 0x1000
    extern char __bss_end[];
}

static size_t GetTotalUsablePageCount(const SystemMemoryLayout& layout) noexcept {
    size_t count = 0;

    for (const MemoryRange& range : layout.available) {
        count += range.size() / x64::kPageSize;
    }

    for (const MemoryRange& range : layout.reclaimable) {
        count += range.size() / x64::kPageSize;
    }

    return count;
}

PhysicalAllocator::PhysicalAllocator(SystemMemoryLayout *layout) noexcept
    : mLayout(layout)
{
    size_t pages = GetTotalUsablePageCount(*mLayout);

    // number of pages required to store the bitmap
    size_t size = pages / (x64::kPageSize * CHAR_BIT);

    // find first available memory range
    MemoryRange range = mLayout->available[0];

    KM_CHECK(range.size() >= size, "Not enough memory to store bitmap.");

    mBitMap = range.front.as<uint8_t>();
    mCount = size;
    memset(mBitMap, 0, size);

    // TODO: wrong

    // mark the bitmap as used
    markUsed(MemoryRange(range.front, range.front.address + size));

    // mark kernel sections as used
    markUsed(MemoryRange((uintptr_t)__text_start, (uintptr_t)__text_end));
    markUsed(MemoryRange((uintptr_t)__rodata_start, (uintptr_t)__rodata_end));
    markUsed(MemoryRange((uintptr_t)__data_start, (uintptr_t)__data_end));
    markUsed(MemoryRange((uintptr_t)__bss_start, (uintptr_t)__bss_end));
}

void PhysicalAllocator::markPagesUsed(size_t first, size_t last) noexcept {
    for (uintptr_t i = first; i < last; i++) {
        mBitMap[i / CHAR_BIT] |= 1ull << (i % CHAR_BIT);
    }
}

void PhysicalAllocator::markUsed(MemoryRange) noexcept {
    // TODO: wrong
    KM_PANIC("Not implemented.");
}

PhysicalAddress PhysicalAllocator::alloc4k(size_t) noexcept {
    // find free range in bitmap
    KM_PANIC("Not implemented.");
}

void PhysicalAllocator::release4k(PhysicalAddress, size_t) noexcept {
    KM_PANIC("Not implemented.");
}

PageAllocator::PageAllocator(PhysicalAllocator *pmm) noexcept
    : mMemory(pmm)
    , mPageMap(mMemory->alloc4k(sizeof(x64::PageMapLevel4) / x64::kPageSize).as<x64::PageMapLevel4>())
{ }

PhysicalAddress PageAllocator::toPhysical(VirtualAddress) const noexcept {
    KM_PANIC("Not implemented.");
}

VirtualAddress PageAllocator::toVirtual(PhysicalAddress) const noexcept {
    KM_PANIC("Not implemented.");
}

void PageAllocator::map4k(VirtualAddress, PhysicalAddress, size_t, PageFlags) noexcept {
    KM_PANIC("Not implemented.");
}
