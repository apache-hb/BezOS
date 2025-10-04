#include "memory/detail/mapping_lookup_cache.hpp"

using km::detail::MappingLookupCache;

OsStatus MappingLookupCache::addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    MapEntry entry {
        .vaddr = mapping.vaddr,
        .count = mapping.size / x64::kPageSize
    };
    sm::PhysicalAddress paddr = mapping.paddr.address;
    return mCache.insert(paddr, entry);
}

OsStatus MappingLookupCache::removeMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    if (auto *it = mCache.find(mapping.paddr.address)) {
        if (it->count * x64::kPageSize != mapping.size || it->vaddr != mapping.paddr.address) {
            return OsStatusInvalidInput;
        }

        mCache.remove(mapping.paddr.address);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

sm::VirtualAddress MappingLookupCache::find(sm::PhysicalAddress paddr) const noexcept [[clang::nonblocking]] {
    auto it = mCache.lowerBound(paddr);
    if (it != mCache.begin()) {
        --it;
    }

    const auto& [key, entry] = *it;
    if (key > paddr || key + (entry.count * x64::kPageSize) <= paddr) {
        return sm::VirtualAddress::invalid();
    }

    uintptr_t slide = key.address - entry.vaddr.address;
    sm::VirtualAddress result = { paddr.address - slide };
    return result;
}
