#pragma once

#include "allocator/allocator.hpp"
#include "boot.hpp"

#include "std/vector.hpp"

namespace km {
    static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

    namespace detail {
        void SortMemoryRanges(std::span<MemoryRange> ranges);
        void MergeMemoryRanges(stdx::Vector<MemoryRange>& ranges);
    }

    constexpr bool IsLowMemory(km::MemoryRange range) {
        return range.front < kLowMemory;
    }

    struct SystemMemoryLayout {
        stdx::Vector<MemoryRange> available;
        stdx::Vector<MemoryRange> reclaimable;

        stdx::Vector<boot::MemoryRegion> reserved;

        void reclaimBootMemory();

        sm::Memory availableMemory() const;
        sm::Memory usableMemory() const;
        sm::Memory reclaimableMemory() const;

        size_t count() const { return available.count() + reclaimable.count(); }

        static SystemMemoryLayout from(std::span<const boot::MemoryRegion> memmap, mem::IAllocator *allocator [[gnu::nonnull]]);
    };
}
