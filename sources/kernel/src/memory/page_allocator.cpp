#include "memory/page_allocator.hpp"

#include "memory/layout.hpp"
#include "std/inlined_vector.hpp"
#include "std/spinlock.hpp"

#include <cstring>

using namespace km;

/// page allocator

MemoryRange PageAllocator::alloc4k(size_t count) [[clang::allocating]] {
    TlsfAllocation allocation = pageAlloc(count);
    if (allocation.isNull()) {
        return MemoryRange{};
    }

    return allocation.range();
}

TlsfAllocation PageAllocator::pageAlloc(size_t count) [[clang::allocating]] {
    return aligned_alloc(alignof(x64::page), count * x64::kPageSize);
}

TlsfAllocation PageAllocator::aligned_alloc(size_t align, size_t size) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.aligned_alloc(align, size);
}

OsStatus PageAllocator::splitv(TlsfAllocation ptr, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.splitv(ptr, points, results);
}

OsStatus PageAllocator::split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo [[gnu::nonnull]], TlsfAllocation *hi [[gnu::nonnull]]) [[clang::allocating]] {
    stdx::LockGuard guard(mLock);
    return mMemoryHeap.split(allocation, midpoint, lo, hi);
}

void PageAllocator::free(TlsfAllocation allocation) noexcept [[clang::nonallocating]] {
    stdx::LockGuard guard(mLock);
    mMemoryHeap.free(allocation);
}

OsStatus PageAllocator::reserve(MemoryRange range, TlsfAllocation *allocation) {
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
    mMemoryHeap.freeAddress(range.front);
}

PageAllocatorStats PageAllocator::stats() noexcept {
    stdx::LockGuard guard(mLock);
    return {
        mMemoryHeap.stats(),
    };
}

OsStatus PageAllocator::create(std::span<const boot::MemoryRegion> memmap, PageAllocator *allocator [[clang::noescape, gnu::nonnull]]) [[clang::allocating]] {
    km::TlsfHeap memoryHeap;

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

    if (OsStatus status = km::TlsfHeap::create(memory, &memoryHeap)) {
        return status;
    }

    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

    allocator->mMemoryHeap = std::move(memoryHeap);

    CLANG_DIAGNOSTIC_POP();

    return OsStatusSuccess;
}
