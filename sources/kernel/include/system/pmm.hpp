#pragma once

#include "log.hpp"
#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "util/absl.hpp"

#include <atomic>

namespace sys2 {
    struct MemorySegmentStats {
        km::MemoryRange range;
        uint8_t owners;
    };

    struct MemorySegment {
        std::atomic<uint8_t> mOwners;
        km::TlsfAllocation mHandle;

        MemorySegment(km::TlsfAllocation handle)
            : MemorySegment(1, handle)
        { }

        MemorySegment(uint8_t owners, km::TlsfAllocation handle)
            : mOwners(owners)
            , mHandle(handle)
        { }

        MemorySegment(MemorySegment&& other)
            : mOwners(other.mOwners.load())
            , mHandle(std::move(other.mHandle))
        { }

        MemorySegmentStats stats() const noexcept [[clang::nonallocating]] {
            return MemorySegmentStats {
                .range = mHandle.range(),
                .owners = mOwners.load(std::memory_order_relaxed),
            };
        }
    };

    struct MemoryManagerStats {
        km::TlsfHeapStats heapStats;
        size_t segments;

        constexpr size_t controlMemory() const noexcept [[clang::nonallocating]] {
            return segments * sizeof(MemorySegment);
        }
    };

    class MemoryManager {
        using Counter = std::atomic<uint8_t>;
        using Map = sm::BTreeMap<km::PhysicalAddress, MemorySegment>;
        using Iterator = typename Map::iterator;

        Map mSegments;
        km::TlsfHeap mHeap;

        MemoryManager(km::TlsfHeap&& heap) noexcept
            : mHeap(std::move(heap))
        { }

        enum ReleaseSide {
            kLow,
            kHigh,
        };

        [[nodiscard]]
        OsStatus splitSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side);

        [[nodiscard]]
        OsStatus retainSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side);

        bool releaseEntry(Iterator it);
        bool releaseSegment(sys2::MemorySegment& segment);

        void addSegment(MemorySegment&& segment);

        [[nodiscard]]
        OsStatus releaseRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining);

        [[nodiscard]]
        OsStatus retainRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining);

    public:
        MemoryManager() = default;

        [[nodiscard]]
        OsStatus retain(km::MemoryRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus release(km::MemoryRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus allocate(size_t size, km::MemoryRange *range) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        MemoryManagerStats stats() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]];

#if __STDC_HOSTED__
        void TESTING_dumpStruct() {
            for (const auto& [address, segment] : mSegments) {
                KmDebugMessage("Segment: ", address, ": ", segment.mHandle.range(), " (owners: ", segment.mOwners.load(), ")\n");
            }
        }

        void TESTING_dumpRange(km::MemoryRange range) {
            auto begin = mSegments.lower_bound(range.front);
            auto end = mSegments.lower_bound(range.back);
            if (end != mSegments.end()) {
                ++end;
            }
            KmDebugMessage("Dumping range: ", range, "\n");
            for (auto it = begin; it != end;) {
                auto& segment = it->second;
                KmDebugMessage("Segment: ", it->first, ": ", segment.mHandle.range(), " (owners: ", segment.mOwners.load(), ")\n");

                ++it;
            }
        }
#endif
    };
}
