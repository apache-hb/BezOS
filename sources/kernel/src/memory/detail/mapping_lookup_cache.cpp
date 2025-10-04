#include "memory/detail/mapping_lookup_cache.hpp"

using km::detail::MappingLookupCache;

OsStatus MappingLookupCache::addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    MapEntry entry {
        .paddr = mapping.paddr.address,
        .count = mapping.size / x64::kPageSize
    };
    sm::VirtualAddress vaddr = mapping.vaddr;
    return mCache.insert(vaddr, entry);
}

OsStatus MappingLookupCache::removeMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    if (auto *it = mCache.find(mapping.vaddr)) {
        if (it->count * x64::kPageSize != mapping.size || it->paddr != mapping.paddr.address) {
            return OsStatusInvalidInput;
        }

        mCache.remove(mapping.vaddr);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

sm::PhysicalAddress MappingLookupCache::find(sm::VirtualAddress vaddr) noexcept [[clang::nonblocking]] {
    auto it = mCache.lowerBound(vaddr);
    if (it != mCache.begin()) {
        --it;
    }

    const auto& [key, entry] = *it;
    if (key > vaddr || key + (entry.count * x64::kPageSize) <= vaddr) {
        return sm::PhysicalAddress::invalid();
    }

    uintptr_t slide = key.address - entry.paddr.address;
    sm::PhysicalAddress paddr = { vaddr.address - slide };
    return paddr;
}
