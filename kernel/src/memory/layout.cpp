#include "memory/layout.hpp"

#include "log.hpp"
#include "panic.hpp"

#include <algorithm>

#include <string.h>

using namespace km;

void detail::SortMemoryRanges(std::span<MemoryRange> ranges) {
    std::sort(ranges.begin(), ranges.end(), [](const MemoryRange& a, const MemoryRange& b) {
        return a.front < b.front;
    });
}

void detail::MergeMemoryRanges(stdx::Vector<MemoryRange>& ranges) {
    for (size_t i = 0; i < ranges.count(); i++) {
        MemoryRange& range = ranges[i];
        for (size_t j = i + 1; j < ranges.count(); j++) {
            MemoryRange& next = ranges[j];
            if (range.intersects(next) || adjacent(range, next)) {
                range = merge(range, next);
                ranges.remove(j);
                j--;
            }
        }
    }
}

SystemMemoryLayout SystemMemoryLayout::from(std::span<const boot::MemoryRegion> memmap, mem::IAllocator *allocator) {
    stdx::Vector<MemoryRange> freeMemory(allocator);
    stdx::Vector<MemoryRange> reclaimable(allocator);
    stdx::Vector<boot::MemoryRegion> reservedMemory(allocator);

    for (size_t i = 0; i < memmap.size(); i++) {
        boot::MemoryRegion entry = memmap[i];

        if (entry.range.isEmpty()) {
            KmDebugMessage("[MEM] Memory range ", i, " is empty.\n");
            continue;
        }

        switch (entry.type) {
        case boot::MemoryRegion::eUsable:
            freeMemory.add(entry.range);
            break;
        case boot::MemoryRegion::eBootloaderReclaimable:
            reclaimable.add(entry.range);
            break;
        default:
            reservedMemory.add(entry);
            break;
        }
    }

    detail::SortMemoryRanges(freeMemory);
    detail::SortMemoryRanges(reclaimable);

    std::sort(reservedMemory.begin(), reservedMemory.end(), [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
        return a.range.front < b.range.front;
    });

    // Only merge adjacent free ranges, the others are not needed
    detail::MergeMemoryRanges(freeMemory);

    uint64_t usableMemory = 0;
    for (MemoryRange range : freeMemory) {
        usableMemory += range.size();
    }

    KM_CHECK(usableMemory != 0, "No usable memory found.");

    return SystemMemoryLayout { std::move(freeMemory), std::move(reclaimable), std::move(reservedMemory) };
}

void SystemMemoryLayout::reclaimBootMemory() {
    available.addRange(reclaimable);

    reclaimable.clear();

    detail::SortMemoryRanges(available);
    detail::MergeMemoryRanges(available);
}

sm::Memory SystemMemoryLayout::availableMemory() const {
    return usableMemory() + reclaimableMemory();
}

sm::Memory SystemMemoryLayout::usableMemory() const {
    size_t total = 0;
    for (MemoryRange range : available) {
        total += range.size();
    }

    return sm::bytes(total);
}

sm::Memory SystemMemoryLayout::reclaimableMemory() const {
    size_t total = 0;
    for (MemoryRange range : reclaimable) {
        total += range.size();
    }

    return sm::bytes(total);
}
