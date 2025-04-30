#include "memory/page_allocator.hpp"

#include "debug/debug.hpp"
#include "memory/layout.hpp"
#include "std/inlined_vector.hpp"

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

MemoryRange PageAllocator::alloc4k(size_t count) [[clang::allocating]] {
    TlsfAllocation allocation = pageAlloc(count);
    if (allocation.isNull()) {
        return MemoryRange{};
    }

    return allocation.range();
}

TlsfAllocation PageAllocator::pageAlloc(size_t count) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.aligned_alloc(alignof(x64::page), count * x64::kPageSize);
}

OsStatus PageAllocator::splitv(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.splitv(ptr, points, results);
}

void PageAllocator::release(TlsfAllocation allocation) noexcept [[clang::nonallocating]] {
    stdx::LockGuard guard(mLock);
    mMemoryHeap.free(allocation);
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

OsStatus PageAllocator::create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator) [[clang::allocating]] {
    km::TlsfHeap memoryHeap;

    sm::InlinedVector<MemoryRange, 16> memory;

    for (boot::MemoryRegion region : memmap) {
        if (!region.isUsable())
            continue;

        if (region.range.isAfter(kLowMemory)) {
            KM_ASSERT(memory.add(region.range) == OsStatusSuccess);
        } else if (region.range.contains(kLowMemory)) {
            auto [low, high] = split(region.range, kLowMemory);
            KM_ASSERT(memory.add(high) == OsStatusSuccess);
        }
    }

    if (OsStatus status = km::TlsfHeap::create(memory, &memoryHeap)) {
        return status;
    }

    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

    allocator->mMemoryHeap = std::move(memoryHeap);

    CLANG_DIAGNOSTIC_POP();

    return OsStatusSuccess;
}
