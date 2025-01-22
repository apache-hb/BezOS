#include "memory/page_allocator.hpp"

#include "arch/paging.hpp"

#include "memory/layout.hpp"
#include "panic.hpp"

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

    extern char __kernel_start[];
    extern char __kernel_end[];
}

/// page allocator

static size_t GetBitmapSize(const SystemMemoryLayout *layout) {
    size_t size = 0;

    for (MemoryRange entry : layout->available) {
        size += detail::GetRangeBitmapSize(entry);
    }

    for (MemoryRange entry : layout->reclaimable) {
        size += detail::GetRangeBitmapSize(entry);
    }

    return size;
}

static constexpr PhysicalAddress kLowMemory = sm::megabytes(1).bytes();

static PhysicalAddress FindFreeSpace(const SystemMemoryLayout *layout, size_t bitmapSize) {
    // Find any memory range that is large enough to hold the bitmap
    // and is not in the first 1MB of memory.

    for (MemoryRange entry : layout->available) {
        MemoryRange usable = intersection(entry, { kLowMemory, UINTPTR_MAX });
        if (usable.size() >= bitmapSize) {
            return usable.front;
        }
    }

    return nullptr;
}

void detail::BuildMemoryRanges(stdx::Vector<RegionBitmapAllocator>& allocators, stdx::Vector<RegionBitmapAllocator>& lowMemory, const SystemMemoryLayout *layout, PhysicalAddress bitmap, uintptr_t hhdmOffset) {
    size_t offset = 0;

    auto newRegion = [&](MemoryRange range) {
        RegionBitmapAllocator result{range, (uint8_t*)(bitmap + offset + hhdmOffset).address};
        offset += detail::GetRangeBitmapSize(range);
        return result;
    };

    // Create an allocator for each memory range
    for (MemoryRange entry : layout->available) {
        // If the range straddles the 1MB boundary, split it
        if (entry.contains(kLowMemory)) {
            auto [low, high] = split(entry, kLowMemory);
            lowMemory.add(newRegion(low));
            allocators.add(newRegion(high));
        } else if (entry.isBefore(kLowMemory)) {
            lowMemory.add(newRegion(entry));
        } else {
            allocators.add(newRegion(entry));
        }
    }
}

static MemoryRange GetBitmapRange(const SystemMemoryLayout *layout) {
    // Get the size of the bitmap and round up to the nearest page.
    size_t size = sm::roundup(GetBitmapSize(layout), x64::kPageSize);

    // Allocate bitmap space
    PhysicalAddress bitmap = FindFreeSpace(layout, size);
    KM_CHECK(bitmap != nullptr, "Failed to find space for bitmap");

    return {bitmap, bitmap + size};
}

PageAllocator::PageAllocator(const SystemMemoryLayout *layout, uintptr_t hhdmOffset, mem::IAllocator *allocator)
    : mAllocators(allocator)
    , mLowMemory(allocator)
    , mBitmapMemory(GetBitmapRange(layout))
{
    memset((void*)(mBitmapMemory.front.address + hhdmOffset), 0, mBitmapMemory.size());

    detail::BuildMemoryRanges(mAllocators, mLowMemory, layout, mBitmapMemory.front, hhdmOffset);

    // Mark the bitmap memory as used
    markUsed(mBitmapMemory);
}

void PageAllocator::rebuild() {
    detail::MergeAdjacentAllocators(mAllocators);
    detail::MergeAdjacentAllocators(mLowMemory);
}

PhysicalAddress PageAllocator::alloc4k(size_t count) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (PhysicalAddress addr = allocator.alloc4k(count); addr != nullptr) {
            return addr;
        }
    }

    return nullptr;
}

void PageAllocator::release(MemoryRange range) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        allocator.release(range);
    }
}

PhysicalAddress PageAllocator::lowMemoryAlloc4k() {
    for (RegionBitmapAllocator& allocator : mLowMemory) {
        if (PhysicalAddress addr = allocator.alloc4k(1); addr != nullptr) {
            return addr;
        }
    }

    return nullptr;
}

void PageAllocator::markUsed(MemoryRange range) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        allocator.markUsed(range);
    }

    for (RegionBitmapAllocator& allocator : mLowMemory) {
        allocator.markUsed(range);
    }
}
