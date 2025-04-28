#pragma once

#include "log.hpp"
#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "std/rcuptr.hpp"
#include "system/detail/range_table.hpp"
#include "system/vmem.hpp"
#include "util/absl.hpp"

#include <atomic>

namespace sys2 {
    struct MemorySegmentStats {
        km::MemoryRange range;
        uint8_t owners;
    };

    struct MemorySegment {
        std::atomic<uint8_t> owners;
        km::TlsfAllocation handle;
        sm::RcuSharedPtr<IMemoryObject> object;

        MemorySegment(km::TlsfAllocation handle)
            : MemorySegment(1, handle)
        { }

        MemorySegment(uint8_t owners, km::TlsfAllocation handle)
            : owners(owners)
            , handle(handle)
        { }

        MemorySegment(MemorySegment&& other)
            : owners(other.owners.load())
            , handle(std::move(other.handle))
        { }

        MemorySegmentStats stats() const noexcept [[clang::nonallocating]] {
            return MemorySegmentStats {
                .range = handle.range(),
                .owners = owners.load(std::memory_order_relaxed),
            };
        }

        km::MemoryRange range() const noexcept [[clang::nonallocating]] {
            return handle.range();
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
        using Table = sys2::detail::RangeTable<MemorySegment>;
        using Map = typename Table::Map;
        using Iterator = typename Map::iterator;

        Table mTable;
        km::TlsfHeap mHeap;

        Map& segments() noexcept {
            return mTable.segments();
        }

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

        void retainEntry(Iterator it);
        void retainEntry(km::PhysicalAddress address);

        bool releaseEntry(km::PhysicalAddress address);
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
        OsStatus allocate(size_t size, size_t align, km::MemoryRange *range) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::TlsfHeap&& heap, MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        MemoryManagerStats stats() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]];

        void validate() {
            bool ok = true;
            for (const auto& [address, segment] : segments()) {
                if (address != segment.range().back) {
                    KmDebugMessage("Invalid address for Segment: ", segment.range(), " (owners: ", segment.owners.load(), ")\n");
                    KmDebugMessage("Invalid address for Segment address: ", address, " != ", segment.range().back, "\n");
                    KmDebugMessage("Invalid address for Segment range: ", segment.range(), "\n");
                    ok = false;
                }

                if (segment.owners.load() == 0) {
                    KmDebugMessage("Unowned Segment: ", segment.range(), " (owners: ", segment.owners.load(), ")\n");
                    ok = false;
                }

                if (km::alignedOut(segment.range(), x64::kPageSize) != segment.range()) {
                    KmDebugMessage("Misaligned Segment: ", segment.range(), " (owners: ", segment.owners.load(), ")\n");
                    ok = false;
                }
            }
            KM_ASSERT(ok);
        }

        void dump() {
            for (const auto& [_, segment] : segments()) {
                KmDebugMessage("Segment: ", segment.range(), " (owners: ", segment.owners.load(), ")\n");
            }
        }
    };
}
