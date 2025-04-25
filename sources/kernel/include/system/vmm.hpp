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

        void addSegment(AddressSegment &&segment) noexcept [[clang::allocating]];

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
        static OsStatus create(const km::PageBuilder *pm, km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]];
    };
}
