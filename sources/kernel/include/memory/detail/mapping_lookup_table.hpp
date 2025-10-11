#pragma once

#include "std/std.hpp"

#include "common/physical_address.hpp"
#include "common/virtual_address.hpp"

#include "memory/layout.hpp"
#include "std/container/static_flat_map.hpp"

namespace km::detail {
    class MappingLookupTable {
        struct MapEntry {
            sm::VirtualAddress vaddr;
            uint32_t count;
            uint32_t refs;
        };

        using Cache = sm::StaticFlatMap<sm::PhysicalAddress, MapEntry>;

        Cache mCache;

    public:
        MappingLookupTable() noexcept [[clang::nonallocating]] = default;

        MappingLookupTable(void *storage [[gnu::nonnull]], size_t size) noexcept [[clang::nonallocating]]
            : mCache(storage, size)
        { }

        OsStatus addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        OsStatus removeMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        OsStatus reclaimMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]];

        sm::VirtualAddress find(sm::PhysicalAddress vaddr) const noexcept [[clang::nonblocking]];

        void retain(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]];
        void release(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]];
    };
}
