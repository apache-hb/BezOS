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
        km::AddressMapping mapping;
    };

    class AddressSpaceManager {
        using Map = sm::BTreeMap<const void *, AddressSegment>;

        Map mSegments;
        km::PageTables mPageTables;
        km::TlsfHeap mHeap;

        AddressSpaceManager(km::PageTables&& ptes, km::TlsfHeap &&heap) noexcept
            : mPageTables(std::move(ptes))
            , mHeap(std::move(heap))
        { }

    public:
        [[nodiscard]]
        OsStatus map(MemoryManager *manager, km::PageFlags flags, km::MemoryType type, km::AddressMapping *mapping) [[clang::allocating]];

        [[nodiscard]]
        OsStatus unmap(MemoryManager *manager, km::AddressMapping mapping) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::AddressMapping pteMemory, km::PageFlags flags, km::VirtualRange vmem, AddressSpaceManager *manager) [[clang::allocating]];
    };
}
