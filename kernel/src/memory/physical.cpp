#include "memory/physical.hpp"

#include "util/memory.hpp"

#include "kernel.hpp"

#include <limine.h>

#include <algorithm>
#include <string.h>

using namespace km;

static stdx::StringView GetMemoryMapTypeName(uint64_t type) {
    static stdx::StringView const kMemoryMapTypeNames[] = {
        [LIMINE_MEMMAP_USABLE]                 = "Usable",
        [LIMINE_MEMMAP_RESERVED]               = "Reserved",
        [LIMINE_MEMMAP_ACPI_RECLAIMABLE]       = "ACPI Reclaimable",
        [LIMINE_MEMMAP_ACPI_NVS]               = "ACPI NVS",
        [LIMINE_MEMMAP_BAD_MEMORY]             = "Bad Memory",
        [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader Reclaimable",
        [LIMINE_MEMMAP_KERNEL_AND_MODULES]     = "Kernel and Modules",
        [LIMINE_MEMMAP_FRAMEBUFFER]            = "Framebuffer",
    };

    if (type < countof(kMemoryMapTypeNames))
        return kMemoryMapTypeNames[type];

    return "Unknown";
}

static MemoryRange GetMemoryRange(ErrorList& errors, uint64_t i, const limine_memmap_entry *entry [[gnu::nonnull]]) {
    uintptr_t front = entry->base;
    uintptr_t back;
    if (__builtin_add_overflow(front, entry->length, &back)) {
        back = UINTPTR_MAX; // overflow
        errors.add("Memory range ends outside of addressable space.", i);
    }

    return MemoryRange { front, back };
}

void PhysicalMemoryLayout::setup(ErrorList& errors, const limine_memmap_response *memmap) noexcept {
    KmDebugMessage("[INIT] ", memmap->entry_count, " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size         | Type\n");
    KmDebugMessage("|-------+--------------------+--------------+-----------------------\n");

    bool usableRangesOverflow = false;
    bool reservedRangesOverflow = false;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        KmDebugMessage("| ", Int(i).pad(3, '0'), "   | ");
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry == NULL) {
            errors.add("Null memory map entry.", i);
            KmDebugMessage("NULL             |         NULL | NULL\n");
            continue;
        }

        MemoryRange range = GetMemoryRange(errors, i, entry);

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            if (!mFreeMemory.add(range) && !usableRangesOverflow) {
                usableRangesOverflow = true;
                errors.add("Too many usable memory ranges.", i);
            }
        } else {
            if (!mReservedMemory.add(range) && !reservedRangesOverflow) {
                reservedRangesOverflow = true;
                errors.add("Too many reserved memory ranges.", i);
            }
        }

        KmDebugMessage(Hex(entry->base).pad(16, '0'), " | ", Int(entry->length).pad(12, ' '), " | ", GetMemoryMapTypeName(entry->type), "\n");
    }
}

static void EraseEmptyRanges(ErrorList& errors, stdx::StaticVector<MemoryRange, 64>& ranges) noexcept {
    for (ssize_t i = ranges.count() - 1; i >= 0; i--) {
        if (ranges[i].size() == 0) {
            errors.add("Zero sized memory range.", i);
            ranges.remove(i);
        }
    }
}

static void MergeMemoryRanges(ErrorList& errors, stdx::StaticVector<MemoryRange, 64>& ranges, stdx::StringView message) noexcept {
    for (ssize_t i = 1; i < ranges.count(); i++) {
        auto mergeCurrentRange = [&] {
            ranges[i - 1].back = ranges[i].back;

            ranges.remove(i);
            i -= 1;
        };

        if (ranges[i - 1].back > ranges[i].front) {
            errors.add(message, i);
            mergeCurrentRange();
        } else if (ranges[i - 1].back == ranges[i].front) {
            // if the ranges are exactly adjacent, merge them without emitting a warning
            mergeCurrentRange();
        }
    }
}

static void SortMemoryRanges(stdx::StaticVector<MemoryRange, 64>& ranges) noexcept {
    std::sort(ranges.begin(), ranges.end(), [](const MemoryRange& a, const MemoryRange& b) {
        return a.front < b.front;
    });
}

static bool RangesOverlap(MemoryRange lhs, MemoryRange rhs) noexcept {
    return lhs.front <= rhs.back && rhs.front <= lhs.back;
}

void PhysicalMemoryLayout::sanitize(ErrorList& errors) noexcept {
    /// find all zero sized memory ranges and remove them
    EraseEmptyRanges(errors, mFreeMemory);
    EraseEmptyRanges(errors, mReservedMemory);

    if (mFreeMemory.isEmpty()) {
        errors.fatal("No usable memory ranges found.", 0);
        return;
    }

    /// sort all memory ranges by their front address
    SortMemoryRanges(mFreeMemory);
    SortMemoryRanges(mReservedMemory);

    /// find overlapping memory ranges and merge them
    MergeMemoryRanges(errors, mFreeMemory, "Memory range overlaps with previous range.");
    MergeMemoryRanges(errors, mReservedMemory, "Reserved memory range overlaps with previous range.");

    // if any reserved memory ranges overlap with usable memory ranges,
    // remove the overlapping sections in the usable memory ranges.

    ssize_t usableIndex = 0;
    uintptr_t usableStart = mFreeMemory[usableIndex].front;

    for (ssize_t i = 0; i < mReservedMemory.count(); i++) {
        uintptr_t reservedStart = mReservedMemory[i].front;
        uintptr_t reservedEnd = mReservedMemory[i].back;

        while (usableIndex < mFreeMemory.count() && mFreeMemory[usableIndex].back <= reservedStart) {
            usableStart = mFreeMemory[usableIndex].back;
            usableIndex += 1;
        }

        if (usableIndex >= mFreeMemory.count()) {
            break;
        }

        uintptr_t usableEnd = mFreeMemory[usableIndex].back;

        if (usableEnd <= reservedStart) {
            continue;
        }

        bool overlap0 = false;
        bool overlap1 = false;

        if (usableStart < reservedStart) {
            mFreeMemory[usableIndex].back = reservedStart;
            usableStart = reservedStart;

            overlap0 = true;
        }

        if (usableEnd > reservedEnd) {
            mFreeMemory.insert(usableIndex + 1, MemoryRange { reservedEnd, usableEnd });

            overlap1 = true;
        }

        if (overlap0 || overlap1) {
            errors.add("Usable memory range overlaps with reserved range.", usableIndex);
        }
    }
}

PhysicalMemoryLayout::PhysicalMemoryLayout(const limine_memmap_response *memmap) noexcept {
    ErrorList errors;

    setup(errors, memmap);
    sanitize(errors);

    if (!errors.isEmpty()) {
        KmDebugMessage("[INIT] Issues detected during memory map setup.\n");
        KmDebugMessage("| Component   | Description\n");
        KmDebugMessage("|-------------+----------------\n");
        for (const ErrorList::Entry& error : errors.errors) {
            KmDebugMessage("| /BOOT/MEM/", error.entry, " | ", error.message, "\n");
        }

        if (errors.flags & ErrorList::eOverflow) {
            KmDebugMessage("[INIT] More than ", errors.errors.capacity(), " issues detected. Some issues may not be displayed.\n");
        }

        if (errors.flags & ErrorList::eFatal) {
            KM_PANIC("Cannot recover from malformed memory map.");
        }
    }

    for (const MemoryRange& range : mReservedMemory) {
        KmDebugMessage("[INIT] Reserved memory range: ", Hex(range.front), " - ", Hex(range.back), "\n");
    }

    for (const MemoryRange& range : mFreeMemory) {
        KmDebugMessage("[INIT] Usable memory range: ", Hex(range.front), " - ", Hex(range.back), "\n");
    }

    uint64_t usableMemory = 0;
    for (const MemoryRange& range : mFreeMemory) {
        usableMemory += range.size();
    }

    KmDebugMessage("[INIT] Usable memory: ", km::bytes(usableMemory), "\n");
}
