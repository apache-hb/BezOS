#include "memory/page_allocator.hpp"

#include "arch/paging.hpp"

#include "kernel.hpp"
#include "memory/layout.hpp"

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

    for (MemoryMapEntry entry : layout->available) {
        size += detail::GetRangeBitmapSize(entry.range);
    }

    for (MemoryMapEntry entry : layout->reclaimable) {
        size += detail::GetRangeBitmapSize(entry.range);
    }

    return size;
}

static PhysicalAddress FindBitmapSpace(const SystemMemoryLayout *layout, size_t bitmapSize) {
    // Find any memory range that is large enough to hold the bitmap
    // and is not in the first 1MB of memory.

    for (MemoryMapEntry entry : layout->available) {
        if (entry.range.size() >= bitmapSize && !entry.range.contains(0x100000)) {
            return entry.range.front;
        }
    }

    return nullptr;
}

static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

PageAllocator::PageAllocator(const SystemMemoryLayout *layout, uintptr_t hhdmOffset) {
    // Get the size of the bitmap and round up to the nearest page.
    size_t bitmapSize = sm::roundup(GetBitmapSize(layout), x64::kPageSize);

    // Allocate bitmap space
    PhysicalAddress bitmap = FindBitmapSpace(layout, bitmapSize);
    KM_CHECK(bitmap != nullptr, "Failed to find space for bitmap");

    size_t offset = 0;

    auto newRegion = [&](MemoryRange range) {
        RegionBitmapAllocator result{range, (uint8_t*)(bitmap + offset + hhdmOffset).address};
        offset += detail::GetRangeBitmapSize(range);
        return result;
    };

    // Create an allocator for each memory range
    for (MemoryMapEntry entry : layout->available) {

        // If the range straddles the 1MB boundary, split it
        if (entry.range.contains(kLowMemory)) {
            auto [low, high] = split(entry.range, kLowMemory);
            mLowMemory.add(newRegion(low));
            mAllocators.add(newRegion(high));
        } else if (entry.range.isBefore(kLowMemory)) {
            mLowMemory.add(newRegion(entry.range));
        } else {
            mAllocators.add(newRegion(entry.range));
        }
    }

    // Mark the bitmap memory as used
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (allocator.contains(bitmap)) {
            allocator.markAsUsed({bitmap, bitmap + bitmapSize});
            break;
        }
    }
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

void PageAllocator::markRangeUsed(MemoryRange range) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (allocator.contains(range.front)) {
            allocator.markAsUsed(range);
        }
    }

    for (RegionBitmapAllocator& allocator : mLowMemory) {
        if (allocator.contains(range.front)) {
            allocator.markAsUsed(range);
        }
    }
}
