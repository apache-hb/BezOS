#pragma once

#include "kernel.hpp"

#include "memory/memory.hpp"

#include "std/static_vector.hpp"

namespace km {
    struct SystemMemoryLayout {
        static constexpr size_t kMaxRanges = 64;
        using FreeMemoryRanges = stdx::StaticVector<MemoryMapEntry, kMaxRanges>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryMapEntry, kMaxRanges>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        void reclaimBootMemory();

        size_t usableMemory() const {
            size_t total = 0;
            for (MemoryMapEntry range : available) {
                total += range.range.size();
            }

            for (MemoryMapEntry range : reclaimable) {
                total += range.range.size();
            }

            return total;
        }

        static SystemMemoryLayout from(const KernelMemoryMap& memmap);
    };
}
