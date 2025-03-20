#include "memory/page_allocator.hpp"

#include "debug/debug.hpp"
#include "memory/layout.hpp"

#include <cstring>

using namespace km;

/// page allocator

PageAllocator::PageAllocator(std::span<const boot::MemoryRegion> memmap) {
    for (boot::MemoryRegion region : memmap) {
        if (!region.isUsable())
            continue;

        release(region.range);
    }
}

PhysicalAddress PageAllocator::alloc4k(size_t count) [[clang::allocating]] {
    MemoryRange range = mMemory.allocate({
        .size = count * x64::kPageSize,
        .align = x64::kPageSize,
    });

    debug::SendEvent(debug::AllocatePhysicalMemory {
        .size = range.size(),
        .address = range.front.address,
        .alignment = x64::kPageSize,
        .tag = 0,
    });

    if (range.isEmpty()) {
        return KM_INVALID_MEMORY;
    }

    return range.front;
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

    debug::SendEvent(debug::ReleasePhysicalMemory {
        .begin = range.front.address,
        .end = range.back.address,
        .tag = 0,
    });
}

PhysicalAddress PageAllocator::lowMemoryAlloc4k() [[clang::allocating]] {
    MemoryRange range = mLowMemory.allocate({
        .size = x64::kPageSize,
        .align = x64::kPageSize,
    });

    debug::SendEvent(debug::AllocatePhysicalMemory {
        .size = range.size(),
        .address = range.front.address,
        .alignment = x64::kPageSize,
        .tag = 0,
    });

    if (range.isEmpty()) {
        return KM_INVALID_MEMORY;
    }

    return range.front;
}

void PageAllocator::reserve(MemoryRange range) {
    mLowMemory.reserve(range);
    mMemory.reserve(range);
}
