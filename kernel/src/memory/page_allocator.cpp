#include "memory/page_allocator.hpp"

#include "memory/layout.hpp"

#include <climits>
#include <cstring>

using namespace km;

/// page allocator

PageAllocator::PageAllocator(const boot::MemoryMap& memmap, mem::IAllocator *allocator)
    : mLowMemory(allocator)
    , mMemory(allocator)
{
    for (boot::MemoryRegion region : memmap.regions) {
        if (!region.isUsable())
            continue;

        release(region.range);
    }
}

PhysicalAddress PageAllocator::alloc4k(size_t count) {
    return mMemory.allocate(count * x64::kPageSize).front;
}

void PageAllocator::release(MemoryRange range) {
    // If the range straddles the 1MB boundary, split it
    if (range.contains(kLowMemory)) {
        auto [low, high] = split(range, kLowMemory);
        mLowMemory.release(low);
        mMemory.release(high);
    } else if (range.isBefore(kLowMemory)) {
        mLowMemory.release(range);
    } else {
        mMemory.release(range);
    }
}

PhysicalAddress PageAllocator::lowMemoryAlloc4k() {
    return mLowMemory.allocate(x64::kPageSize).front;
}

void PageAllocator::markUsed(MemoryRange range) {
    mLowMemory.markUsed(range);
    mMemory.markUsed(range);
}
