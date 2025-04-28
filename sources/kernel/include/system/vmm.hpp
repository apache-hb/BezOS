#pragma once

#include "memory/heap.hpp"
#include "memory/layout.hpp"
#include "memory/memory.hpp"
#include "memory/paging.hpp"
#include "memory/pte.hpp"

#include "util/absl.hpp"

namespace sys2 {
    class MemoryManager;

    struct AddressSegment {
        km::MemoryRange range;
        km::TlsfAllocation allocation;

        km::VirtualRange virtualRange() const noexcept [[clang::nonallocating]] {
            return allocation.range().cast<const void*>();
        }

        km::AddressMapping mapping() const noexcept [[clang::nonallocating]] {
            auto front = allocation.address();
            const void *vaddr = std::bit_cast<const void*>(front);
            return km::MappingOf(range, vaddr);
        }
    };

    struct AddressSpaceManagerStats {
        km::TlsfHeapStats heapStats;
        size_t segments;

        constexpr size_t controlMemory() const noexcept [[clang::nonallocating]] {
            return segments * sizeof(AddressSegment);
        }
    };

    class AddressSpaceManager {
        using Map = sm::BTreeMap<const void *, AddressSegment>;
        using Iterator = typename Map::iterator;

        Map mSegments;
        km::AddressMapping mPteMemory;
        km::PageTables mPageTables;
        km::TlsfHeap mHeap;

        AddressSpaceManager(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::TlsfHeap &&heap) noexcept
            : mPteMemory(pteMemory)
            , mPageTables(pm, mPteMemory, flags)
            , mHeap(std::move(heap))
        { }

        enum ReleaseSide {
            kLow,
            kHigh,
        };

        Iterator eraseSegment(MemoryManager *manager, Iterator it) noexcept [[clang::allocating]];
        void eraseSegment(MemoryManager *manager, km::VirtualRange segment) noexcept [[clang::allocating]];
        void eraseMany(MemoryManager *manager, km::VirtualRange range) noexcept [[clang::allocating]];

        void addSegment(AddressSegment &&segment) noexcept [[clang::allocating]];
        void addNoAccessSegment(km::TlsfAllocation allocation) noexcept [[clang::allocating]];

        /// @param manager The memory manager to use.
        /// @param it The iterator to the segment to map.
        /// @param src The full source range that is being mapped.
        /// @param dst The destination range to map to.
        /// @param flags The page flags to use.
        /// @param type The memory type to use.
        OsStatus mapSegment(
            MemoryManager *manager,
            Iterator it,
            km::VirtualRange src,
            km::VirtualRange dst,
            km::PageFlags flags,
            km::MemoryType type,
            km::TlsfAllocation allocation,
            km::TlsfAllocation *remaining
        ) [[clang::allocating]];

        OsStatus splitSegment(MemoryManager *manager, Iterator it, const void *midpoint, ReleaseSide side) [[clang::allocating]];
        OsStatus unmapSegment(MemoryManager *manager, Iterator it, km::VirtualRange range, km::VirtualRange *remaining) [[clang::allocating]];
    public:
        UTIL_DEFAULT_MOVE(AddressSpaceManager);

        constexpr AddressSpaceManager() noexcept = default;

        [[nodiscard]]
        OsStatus map(MemoryManager *manager, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]];

        [[nodiscard]]
        OsStatus map(MemoryManager *manager, AddressSpaceManager *other, km::VirtualRange range, km::PageFlags flags, km::MemoryType type, km::VirtualRange *mapping) [[clang::allocating]];

        [[nodiscard]]
        OsStatus unmap(MemoryManager *manager, km::VirtualRange range) [[clang::allocating]];

        [[nodiscard]]
        OsStatus querySegment(const void *address, AddressSegment *result) noexcept [[clang::nonallocating]];

        AddressSpaceManagerStats stats() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus destroy(MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]];
    };
}
