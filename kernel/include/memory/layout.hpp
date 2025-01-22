#pragma once

#include "allocator/allocator.hpp"
#include "boot.hpp"

#include "std/vector.hpp"

namespace km {
    struct SystemMemoryLayout {
        static constexpr size_t kMaxRanges = 64;

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

    namespace detail {
        void SortMemoryRanges(std::span<MemoryRange> ranges);
        void MergeMemoryRanges(stdx::Vector<MemoryRange>& ranges);
    }
}
