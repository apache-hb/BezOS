#include "memory/physical.hpp"

#include "util/memory.hpp"

#include "kernel.hpp"

#include <limine.h>

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

struct ErrorList {
    enum { eOverflow = (1 << 1) };
    struct Entry { stdx::StringView message; uint64_t entry; };
    stdx::StaticVector<Entry, 64> errors;
    unsigned flags = 0;

    void add(stdx::StringView message, uint64_t entry) {
        if (!errors.add(Entry { message, entry }))
            flags |= eOverflow;
    }

    bool isEmpty() const noexcept { return errors.isEmpty(); }
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

SystemMemoryLayout SystemMemoryLayout::from(const limine_memmap_response *memmap) noexcept {
    FreeMemoryRanges freeMemory;
    ReservedMemoryRanges reservedMemory;

    ErrorList errors;

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

        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            if (!freeMemory.add(range) && !usableRangesOverflow) {
                usableRangesOverflow = true;
                errors.add("Too many usable memory ranges.", i);
            }
        } else {
            if (!reservedMemory.add(range) && !reservedRangesOverflow) {
                reservedRangesOverflow = true;
                errors.add("Too many reserved memory ranges.", i);
            }
        }

        KmDebugMessage(Hex(entry->base).pad(16, '0'), " | ", Int(entry->length).pad(12, ' '), " | ", GetMemoryMapTypeName(entry->type), "\n");
    }

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
    }

    uint64_t usableMemory = 0;
    for (const MemoryRange& range : freeMemory) {
        usableMemory += range.size();
    }

    KM_CHECK(usableMemory > 0, "No usable memory found.");

    KmDebugMessage("[INIT] Usable memory: ", km::bytes(usableMemory), "\n");

    return SystemMemoryLayout { freeMemory, reservedMemory };
}
