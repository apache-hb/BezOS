#pragma once

#include "kernel.hpp"

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

        sm::Memory availableMemory() const;
        sm::Memory usableMemory() const;
        sm::Memory reclaimableMemory() const;

        static SystemMemoryLayout from(const KernelMemoryMap& memmap);
    };

    namespace detail {
        void SortMemoryRanges(stdx::StaticVector<MemoryMapEntry, SystemMemoryLayout::kMaxRanges>& ranges);
        void MergeMemoryRanges(stdx::StaticVector<MemoryMapEntry, SystemMemoryLayout::kMaxRanges>& ranges);
    }
}
