#include "memory/page_allocator.hpp"

#include "logger/categories.hpp"
#include "memory/layout.hpp"
#include "std/inlined_vector.hpp"
#include "std/spinlock.hpp"

#include <cstring>

using namespace km;

/// page allocator

PmmAllocation PageAllocator::pageAlloc(size_t count) [[clang::allocating]] {
    return aligned_alloc(alignof(x64::page), count * x64::kPageSize);
}

PmmAllocation PageAllocator::aligned_alloc(size_t align, size_t size) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.alignedAlloc(align, size);
}

OsStatus PageAllocator::splitv(PmmAllocation ptr, std::span<const PhysicalAddress> points, std::span<PmmAllocation> results) {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.splitv(ptr, points, results);
}

OsStatus PageAllocator::split(PmmAllocation allocation, PhysicalAddress midpoint, PmmAllocation *lo [[outparam]], PmmAllocation *hi [[outparam]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.split(allocation, midpoint.address, lo, hi);
}

void PageAllocator::free(PmmAllocation allocation) noexcept [[clang::nonallocating]] {
    stdx::LockGuard guard(mLock);
    mMemoryHeap.free(allocation);
}

OsStatus PageAllocator::reserve(MemoryRange range, PmmAllocation *allocation [[outparam]]) {
    if (range.isEmpty() || (range != alignedOut(range, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);
    if (OsStatus status = mMemoryHeap.reserve(range.cast<km::PhysicalAddress>(), allocation)) {
        return status;
    }

    return OsStatusSuccess;
}

void PageAllocator::release(MemoryRange range) {
    stdx::LockGuard guard(mLock);
    MemLog.infof("Releasing memory range: ", range);
    // mMemoryHeap.freeAddress(range.front);
}

PageAllocatorStats PageAllocator::stats() noexcept {
    stdx::LockGuard guard(mLock);
    return {
        mMemoryHeap.stats(),
    };
}

OsStatus PageAllocator::create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator [[outparam]]) [[clang::allocating]] {
    km::PmmHeap memoryHeap;

    sm::InlinedVector<MemoryRange, 16> memory;

    for (boot::MemoryRegion region : memmap) {
        if (!region.isUsable())
            continue;

        if (region.range.isAfter(kLowMemory)) {
            KM_ASSERT(memory.add(region.range) == OsStatusSuccess);
        } else if (region.range.contains(kLowMemory)) {
            auto [low, high] = km::split(region.range, kLowMemory);
            KM_ASSERT(memory.add(high) == OsStatusSuccess);
        }
    }

    if (OsStatus status = km::PmmHeap::create(memory, &memoryHeap)) {
        return status;
    }

    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

    allocator->mMemoryHeap = std::move(memoryHeap);

    CLANG_DIAGNOSTIC_POP();

    return OsStatusSuccess;
}
