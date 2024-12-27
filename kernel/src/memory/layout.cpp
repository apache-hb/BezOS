#include "memory/layout.hpp"

#include "util/memory.hpp"

#include "kernel.hpp"

#include <algorithm>

#include <string.h>

using namespace km;

static stdx::StringView GetMemoryMapTypeName(MemoryMapEntryType type) {
    static constexpr stdx::StringView kMemoryMapTypeNames[] = {
        [(int)MemoryMapEntryType::eUsable]                = "Usable",
        [(int)MemoryMapEntryType::eReserved]              = "Reserved",
        [(int)MemoryMapEntryType::eAcpiReclaimable]       = "ACPI Reclaimable",
        [(int)MemoryMapEntryType::eAcpiNvs]               = "ACPI NVS",
        [(int)MemoryMapEntryType::eBadMemory]             = "Bad Memory",
        [(int)MemoryMapEntryType::eBootloaderReclaimable] = "Bootloader Reclaimable",
        [(int)MemoryMapEntryType::eKernel]                = "Kernel and Modules",
        [(int)MemoryMapEntryType::eFrameBuffer]           = "Framebuffer",
    };

    if ((size_t)type < std::size(kMemoryMapTypeNames))
        return kMemoryMapTypeNames[(size_t)type];

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

SystemMemoryLayout SystemMemoryLayout::from(KernelMemoryMap memmap) {
    FreeMemoryRanges freeMemory;
    FreeMemoryRanges reclaimable;
    ReservedMemoryRanges reservedMemory;

    ErrorList errors;

    KmDebugMessage("[INIT] ", memmap.count(), " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size             | Type\n");
    KmDebugMessage("|-------+--------------------+------------------+-----------------------\n");

    bool usableRangesOverflow = false;
    bool reclaimableRangesOverflow = false;
    bool reservedRangesOverflow = false;

    for (ssize_t i = 0; i < memmap.count(); i++) {
        auto addMemoryRange = [&](MemoryRange range, auto& ranges, stdx::StringView message, bool& overflow) {
            if (!ranges.add(range) && !overflow) {
                overflow = true;
                errors.add(message, i);
            }
        };

        KmDebugMessage("| ", Int(i).pad(3, '0'), "   | ");
        MemoryMapEntry entry = memmap[i];
        MemoryRange range = entry.range;

        switch (entry.type) {
        case MemoryMapEntryType::eUsable:
            addMemoryRange(range, freeMemory, "Too many usable memory ranges.", usableRangesOverflow);
            break;
        case MemoryMapEntryType::eBootloaderReclaimable:
            addMemoryRange(range, reclaimable, "Too many reclaimable memory ranges.", reclaimableRangesOverflow);
            break;
        default:
            addMemoryRange(range, reservedMemory, "Too many reserved memory ranges.", reservedRangesOverflow);
            break;
        }

        KmDebugMessage(range.front, " | ", lpad(16) + sm::bytes(range.size()), " | ", GetMemoryMapTypeName(entry.type), "\n");
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
