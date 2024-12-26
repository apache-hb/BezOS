#include "memory/layout.hpp"

#include "util/memory.hpp"

#include "kernel.hpp"

#include <algorithm>
#include <limine.h>

#include <string.h>

using namespace km;

static stdx::StringView GetMemoryMapTypeName(uint64_t type) {
    static constexpr stdx::StringView kMemoryMapTypeNames[] = {
        [LIMINE_MEMMAP_USABLE]                 = "Usable",
        [LIMINE_MEMMAP_RESERVED]               = "Reserved",
        [LIMINE_MEMMAP_ACPI_RECLAIMABLE]       = "ACPI Reclaimable",
        [LIMINE_MEMMAP_ACPI_NVS]               = "ACPI NVS",
        [LIMINE_MEMMAP_BAD_MEMORY]             = "Bad Memory",
        [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader Reclaimable",
        [LIMINE_MEMMAP_KERNEL_AND_MODULES]     = "Kernel and Modules",
        [LIMINE_MEMMAP_FRAMEBUFFER]            = "Framebuffer",
    };

    if (type < std::size(kMemoryMapTypeNames))
        return kMemoryMapTypeNames[type];

    return "Unknown";
}

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

static MemoryRange GetMemoryRange(ErrorList& errors, uint64_t i, const limine_memmap_entry *entry [[gnu::nonnull]]) {
    uintptr_t front = entry->base;
    uintptr_t back;
    if (__builtin_add_overflow(front, entry->length, &back)) {
        back = UINTPTR_MAX; // overflow
        errors.add("Memory range ends outside of addressable space.", i);
    }

    return MemoryRange { front, back };
}

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

SystemMemoryLayout SystemMemoryLayout::from(limine_memmap_response memmap) {
    FreeMemoryRanges freeMemory;
    FreeMemoryRanges reclaimable;
    ReservedMemoryRanges reservedMemory;

    ErrorList errors;

    KmDebugMessage("[INIT] ", memmap.entry_count, " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size             | Type\n");
    KmDebugMessage("|-------+--------------------+------------------+-----------------------\n");

    bool usableRangesOverflow = false;
    bool reclaimableRangesOverflow = false;
    bool reservedRangesOverflow = false;

    for (uint64_t i = 0; i < memmap.entry_count; i++) {
        auto addMemoryRange = [&](MemoryRange range, auto& ranges, stdx::StringView message, bool& overflow) {
            if (!ranges.add(range) && !overflow) {
                overflow = true;
                errors.add(message, i);
            }
        };

        KmDebugMessage("| ", Int(i).pad(3, '0'), "   | ");
        struct limine_memmap_entry *entry = memmap.entries[i];
        if (entry == NULL) {
            errors.add("Null memory map entry.", i);
            KmDebugMessage("NULL               |             NULL | NULL\n");
            continue;
        }

        MemoryRange range = GetMemoryRange(errors, i, entry);

        switch (entry->type) {
        case LIMINE_MEMMAP_USABLE:
            addMemoryRange(range, freeMemory, "Too many usable memory ranges.", usableRangesOverflow);
            break;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            addMemoryRange(range, reclaimable, "Too many reclaimable memory ranges.", reclaimableRangesOverflow);
            break;
        default:
            addMemoryRange(range, reservedMemory, "Too many reserved memory ranges.", reservedRangesOverflow);
            break;
        }

        KmDebugMessage(Hex(entry->base).pad(16, '0'), " | ", lpad(16) + sm::bytes(entry->length), " | ", GetMemoryMapTypeName(entry->type), "\n");
    }

    uint64_t usableMemory = 0;
    for (const MemoryRange& range : freeMemory) {
        usableMemory += range.size();
    }

    uint64_t reclaimableMemory = 0;
    for (const MemoryRange& range : reclaimable) {
        reclaimableMemory += range.size();
    }

    if (usableMemory == 0) {
        errors.fatal("No usable memory found.", 0);
    }

    ReportMemoryIssues(errors);

    KmDebugMessage("[INIT] Usable memory: ", sm::bytes(usableMemory), ", Reclaimable memory: ", sm::bytes(reclaimableMemory), "\n");

    return SystemMemoryLayout { freeMemory, reclaimable, reservedMemory };
}

void SystemMemoryLayout::reclaimBootMemory() {
    for (MemoryRange range : reclaimable) {
        available.add(range);
    }

    std::sort(available.begin(), available.end(), [](const MemoryRange& a, const MemoryRange& b) {
        return a.front < b.front;
    });

    // merge adjacent ranges
    for (ssize_t i = 0; i < available.count(); i++) {
        for (ssize_t j = i + 1; j < available.count(); j++) {
            if (available[i].back == available[j].front) {
                available[i].back = available[j].back;
                available.remove(j);
                j--;
            }
        }
    }

    reclaimable.clear();
}
