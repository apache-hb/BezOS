#include "memory/page_allocator.hpp"

#include "memory/layout.hpp"

#include <climits>
#include <cstring>

using namespace km;

/// page allocator

static size_t GetBitmapSize(const boot::MemoryMap& layout) {
    size_t size = 0;

    for (boot::MemoryRegion entry : layout.regions) {
        if (entry.isUsable() || entry.isReclaimable()) {
            size += detail::GetRangeBitmapSize(entry.range);
        }
    }

    return size;
}

void detail::BuildMemoryRanges(RegionList& allocators, RegionList& lowMemory, const boot::MemoryMap& layout, uint8_t *bitmap) {
    size_t offset = 0;

    auto newRegion = [&](MemoryRange range) {
        RegionBitmapAllocator result{range, (bitmap + offset)};
        offset += detail::GetRangeBitmapSize(range);
        return result;
    };

    // Create an allocator for each memory range
    for (boot::MemoryRegion region : layout.regions) {
        if (!region.isUsable())
            continue;

        MemoryRange entry = region.range;

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

void detail::MergeAdjacentAllocators(RegionList& allocators) {
    for (size_t i = 0; i < allocators.count(); i++) {
        RegionBitmapAllocator& allocator = allocators[i];
        for (size_t j = i + 1; j < allocators.count(); j++) {
            RegionBitmapAllocator& next = allocators[j];
            if (allocator.range().intersects(next.range())) {
                allocator.extend(next);
                allocators.remove(j);
                j--;
            }
        }
    }
}

PageAllocator::PageAllocator(const boot::MemoryMap& memmap, mem::IAllocator *allocator)
    : mAllocators(allocator)
    , mLowMemory(allocator)
    , mBitmapMemory((uint8_t*)allocator->allocate(GetBitmapSize(memmap)), allocator)
{
    memset(mBitmapMemory.get(), 0, GetBitmapSize(memmap));

    detail::BuildMemoryRanges(mAllocators, mLowMemory, memmap, mBitmapMemory.get());
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
