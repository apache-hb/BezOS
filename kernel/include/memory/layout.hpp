#pragma once

#include "kernel.hpp"

#include "memory/memory.hpp"

#include "std/static_vector.hpp"

namespace km {
    namespace detail {
        void SortMemoryRanges(stdx::StaticVectorBase<MemoryMapEntry>& ranges);
        void MergeMemoryRanges(stdx::StaticVectorBase<MemoryMapEntry>& ranges);
    }

    struct SystemMemoryLayout {
        static constexpr size_t kMaxRanges = 64;
        using FreeMemoryRanges = stdx::StaticVector<MemoryMapEntry, kMaxRanges>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryMapEntry, kMaxRanges>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        void reclaimBootMemory();

        sm::Memory availableMemory() const {
            return usableMemory() + reclaimableMemory();
        }

        sm::Memory usableMemory() const {
            size_t total = 0;
            for (MemoryMapEntry range : available) {
                total += range.range.size();
            }

            return sm::bytes(total);
        }

        sm::Memory reclaimableMemory() const {
            size_t total = 0;
            for (MemoryMapEntry range : reclaimable) {
                total += range.range.size();
            }

            return sm::bytes(total);
        }

        static SystemMemoryLayout from(const KernelMemoryMap& memmap);
    };
}
