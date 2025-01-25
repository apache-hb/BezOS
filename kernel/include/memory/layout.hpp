#pragma once

#include "allocator/allocator.hpp"
#include "boot.hpp"

#include "std/vector.hpp"
#include <numeric>

namespace km {
    static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

    namespace detail {
        void SortMemoryRanges(std::span<MemoryRange> ranges);
        void MergeMemoryRanges(stdx::Vector<MemoryRange>& ranges);

        inline void SortMemoryRegions(std::span<boot::MemoryRegion> regions) {
            std::sort(regions.begin(), regions.end(), [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
                return a.range.front < b.range.front;
            });
        }
    }

    constexpr bool IsLowMemory(km::MemoryRange range) {
        return range.front < kLowMemory;
    }

    class SystemMemoryMap {
        stdx::Vector<boot::MemoryRegion> regions;

    public:
        SystemMemoryMap(std::span<const boot::MemoryRegion> memmap, mem::IAllocator *allocator [[gnu::nonnull]])
            : regions(memmap.data(), memmap.data() + memmap.size(), allocator)
        {
            detail::SortMemoryRegions(regions);
        }

        size_t count() const { return regions.count(); }

        boot::MemoryRegion *begin() { return regions.begin(); }
        boot::MemoryRegion *end() { return regions.end(); }

        const boot::MemoryRegion *begin() const { return regions.begin(); }
        const boot::MemoryRegion *end() const { return regions.end(); }

        sm::Memory availableMemory() const {
            return std::accumulate(begin(), end(), sm::bytes(0), [](sm::Memory total, const boot::MemoryRegion& entry) {
                return entry.isUsable() ? total + entry.size() : total;
            });
        }

        sm::Memory usableMemory() const {
            return std::accumulate(begin(), end(), sm::bytes(0), [](sm::Memory total, const boot::MemoryRegion& entry) {
                return (entry.type == boot::MemoryRegion::eUsable) ? total + entry.size() : total;
            });
        }

        sm::Memory reclaimableMemory() const {
            return std::accumulate(begin(), end(), sm::bytes(0), [](sm::Memory total, const boot::MemoryRegion& entry) {
                return entry.isReclaimable() ? total + entry.size() : total;
            });
        }
    };

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
