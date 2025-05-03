#pragma once

#include "log.hpp"
#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "std/spinlock.hpp"
#include "system/detail/range_table.hpp"
#include "common/compiler/compiler.hpp"

#include <atomic>

namespace vfs2 {
    class IFileHandle;
}

namespace km {
    class AddressSpace;
    class PageAllocator;
}

namespace sys {
    struct MemorySegmentStats {
        km::MemoryRange range;
        uint8_t owners;
    };

    struct MemorySegment {
        std::atomic<uint8_t> owners;
        km::TlsfAllocation allocation;

        MemorySegment(km::TlsfAllocation handle)
            : MemorySegment(1, handle)
        { }

        MemorySegment(uint8_t owners, km::TlsfAllocation handle)
            : owners(owners)
            , allocation(handle)
        { }

        MemorySegment(MemorySegment&& other)
            : owners(other.owners.load())
            , allocation(std::move(other.allocation))
        { }

        MemorySegmentStats stats() const noexcept [[clang::nonallocating]] {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

            return MemorySegmentStats {
                .range = allocation.range(),
                .owners = owners.load(std::memory_order_relaxed),
            };

            CLANG_DIAGNOSTIC_POP();
        }

        km::MemoryRange range() const noexcept [[clang::nonallocating]] {
            return allocation.range();
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
        using Table = sys::detail::RangeTable<MemorySegment>;
        using Map = typename Table::Map;
        using Iterator = typename Map::iterator;

        stdx::SpinLock mLock;

        Table mTable GUARDED_BY(mLock);
        km::PageAllocator *mHeap;

        Map& segments() noexcept REQUIRES(mLock) {
            return mTable.segments();
        }

        MemoryManager(km::PageAllocator *heap) noexcept
            : mHeap(heap)
        { }

        enum ReleaseSide {
            kLow,
            kHigh,
        };

        [[nodiscard]]
        OsStatus splitSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) REQUIRES(mLock);

        [[nodiscard]]
        OsStatus retainSegment(Iterator it, km::PhysicalAddress midpoint, ReleaseSide side) REQUIRES(mLock);

        void retainEntry(Iterator it) REQUIRES(mLock);
        void retainEntry(km::PhysicalAddress address) REQUIRES(mLock);

        bool releaseEntry(km::PhysicalAddress address) REQUIRES(mLock);
        bool releaseEntry(Iterator it) REQUIRES(mLock);
        bool releaseSegment(sys::MemorySegment& segment) REQUIRES(mLock);

        void addSegment(MemorySegment&& segment) REQUIRES(mLock);

        [[nodiscard]]
        OsStatus releaseRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) REQUIRES(mLock);

        [[nodiscard]]
        OsStatus retainRange(Iterator it, km::MemoryRange range, km::MemoryRange *remaining) REQUIRES(mLock);

    public:
        UTIL_NOCOPY(MemoryManager);

        constexpr MemoryManager(MemoryManager&& other) noexcept
            : mTable(std::move(other.mTable))
            , mHeap(std::move(other.mHeap))
        { }

        constexpr MemoryManager& operator=(MemoryManager&& other) noexcept {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

            mTable = std::move(other.mTable);
            mHeap = std::move(other.mHeap);
            return *this;

            CLANG_DIAGNOSTIC_POP();
        }

        constexpr MemoryManager() noexcept = default;

        [[nodiscard]]
        OsStatus retain(km::MemoryRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus release(km::MemoryRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus allocate(size_t size, size_t align, km::MemoryRange *range) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::PageAllocator *heap, MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        MemoryManagerStats stats() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus querySegment(km::PhysicalAddress address, MemorySegmentStats *stats) noexcept [[clang::nonallocating]];

        void validate() {
            stdx::LockGuard guard(mLock);
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
            stdx::LockGuard guard(mLock);
            for (const auto& [_, segment] : segments()) {
                KmDebugMessage("Segment: ", segment.range(), " (owners: ", segment.owners.load(), ")\n");
            }
        }
    };

    OsStatus MapFileToMemory(vfs2::IFileHandle *file, MemoryManager *mm, km::AddressSpace *pt, size_t offset, size_t size, km::MemoryRange *range);
}
