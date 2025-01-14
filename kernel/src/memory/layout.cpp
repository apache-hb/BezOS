#include "memory/layout.hpp"

#include "log.hpp"
#include "panic.hpp"

#include <algorithm>

#include <string.h>

using namespace km;

struct ErrorList {
    enum { eFatal = (1 << 0), eOverflow = (1 << 1) };
    struct Entry { stdx::StringView message; uint64_t entry; };
    stdx::StaticVector<Entry, 64> errors;
    unsigned flags = 0;

    void add(stdx::StringView message, uint64_t entry) {
        if (!errors.add(Entry { message, entry }))
            flags |= eOverflow;
    }

    void fatal(stdx::StringView message, uint64_t entry) {
        add(message, entry);
        flags |= eFatal;
    }

    bool isEmpty() const { return errors.isEmpty(); }
};

static void ReportMemoryIssues(const ErrorList& errors) {
    if (!errors.isEmpty()) {
        KmDebugMessage("[INIT] Issues detected during memory map setup.\n");
        KmDebugMessage("| Component   | Description\n");
        KmDebugMessage("|-------------+----------------\n");
        for (const ErrorList::Entry& error : errors.errors) {
            KmDebugMessage("| /BOOT/MEM/E", error.entry, " | ", error.message, "\n");
        }

        if (errors.flags & ErrorList::eOverflow) {
            KmDebugMessage("[INIT] More than ", errors.errors.capacity(), " issues detected. Some issues may not be displayed.\n");
        }

        if (errors.flags & ErrorList::eFatal) {
            KM_PANIC("Fatal issues detected during memory map setup.");
        }
    }
}

void detail::SortMemoryRanges(stdx::StaticVector<MemoryMapEntry, SystemMemoryLayout::kMaxRanges>& ranges) {
    std::sort(ranges.begin(), ranges.end(), [](const MemoryMapEntry& a, const MemoryMapEntry& b) {
        return a.range.front < b.range.front;
    });
}

void detail::MergeMemoryRanges(stdx::StaticVector<MemoryMapEntry, SystemMemoryLayout::kMaxRanges>& ranges) {
    for (size_t i = 0; i < ranges.count(); i++) {
        MemoryMapEntry& range = ranges[i];
        for (size_t j = i + 1; j < ranges.count(); j++) {
            MemoryMapEntry& next = ranges[j];
            if (adjacent(range.range, next.range)) {
                range.range = merge(range.range, next.range);
                ranges.remove(j);
                j--;
            }
        }
    }
}

SystemMemoryLayout SystemMemoryLayout::from(const KernelMemoryMap& memmap) {
    FreeMemoryRanges freeMemory;
    FreeMemoryRanges reclaimable;
    ReservedMemoryRanges reservedMemory;

    ErrorList errors;
    bool usableRangesOverflow = false;
    bool reclaimableRangesOverflow = false;
    bool reservedRangesOverflow = false;

    for (size_t i = 0; i < memmap.count(); i++) {
        auto addMemoryRange = [&](MemoryMapEntry range, auto& ranges, stdx::StringView message, bool& overflow) {
            if (!ranges.add(range) && !overflow) {
                overflow = true;
                errors.add(message, i);
            }
        };

        MemoryMapEntry entry = memmap[i];

        if (entry.range.isEmpty()) {
            errors.add("Empty memory range.", i);
            continue;
        }

        switch (entry.type) {
        case MemoryMapEntryType::eUsable:
            addMemoryRange(entry, freeMemory, "Too many usable memory ranges.", usableRangesOverflow);
            break;
        case MemoryMapEntryType::eBootloaderReclaimable:
            addMemoryRange(entry, reclaimable, "Too many reclaimable memory ranges.", reclaimableRangesOverflow);
            break;
        default:
            addMemoryRange(entry, reservedMemory, "Too many reserved memory ranges.", reservedRangesOverflow);
            break;
        }
    }

    detail::SortMemoryRanges(freeMemory);
    detail::SortMemoryRanges(reclaimable);
    detail::SortMemoryRanges(reservedMemory);

    // Only merge adjacent free ranges, the others are not needed
    detail::MergeMemoryRanges(freeMemory);

    uint64_t usableMemory = 0;
    for (const MemoryMapEntry& range : freeMemory) {
        usableMemory += range.range.size();
    }

    if (usableMemory == 0) {
        errors.fatal("No usable memory found.", 0);
    }

    ReportMemoryIssues(errors);

    return SystemMemoryLayout { freeMemory, reclaimable, reservedMemory };
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
    for (MemoryMapEntry range : available) {
        total += range.range.size();
    }

    return sm::bytes(total);
}

sm::Memory SystemMemoryLayout::reclaimableMemory() const {
    size_t total = 0;
    for (MemoryMapEntry range : reclaimable) {
        total += range.range.size();
    }

    return sm::bytes(total);
}
