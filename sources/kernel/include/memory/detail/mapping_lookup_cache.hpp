#pragma once

#include "common/physical_address.hpp"
#include "common/virtual_address.hpp"

#include "memory/layout.hpp"
#include "std/container/static_flat_map.hpp"

namespace km::detail {
    class MappingLookupCache {
        struct MapEntry {
            sm::VirtualAddress vaddr;
            size_t count;
        };

        using Cache = sm::StaticFlatMap<sm::PhysicalAddress, MapEntry>;

        Cache mCache;

    public:
        MappingLookupCache() noexcept [[clang::nonallocating]] = default;

        MappingLookupCache(void *storage [[gnu::nonnull]], size_t size) noexcept [[clang::nonallocating]]
            : mCache(storage, size)
        { }

        OsStatus addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        OsStatus removeMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        sm::VirtualAddress find(sm::PhysicalAddress vaddr) noexcept [[clang::nonblocking]];
    };
}