#pragma once

#include <stddef.h>
#include <stdint.h>

namespace km {
    struct MemoryRange {
        uintptr_t base;
        uint64_t length;
    };

    class PhysicalMemoryLayout {
        static constexpr size_t kMaxFreeMemoryRanges = 128;
        MemoryRange mAvailableMemoryRanges[kMaxFreeMemoryRanges];
        size_t mUsedAvailableMemoryRanges;

    public:
        PhysicalMemoryLayout(const struct limine_memmap_response *memmap);
    };
}
