#include "memory/detail/mapping_lookup_table.hpp"

using km::detail::MappingLookupTable;

OsStatus MappingLookupTable::addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    MapEntry entry {
        .vaddr = mapping.vaddr,
        .count = uint32_t(mapping.size / x64::kPageSize)
    };
    sm::PhysicalAddress paddr = mapping.paddr.address;
    return mCache.insert(paddr, entry);
}

OsStatus MappingLookupTable::removeMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    if (auto *it = mCache.find(mapping.paddr.address)) {
        if (it->count * x64::kPageSize != mapping.size || it->vaddr != mapping.paddr.address) {
            return OsStatusInvalidInput;
        }

        mCache.remove(mapping.paddr.address);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

sm::VirtualAddress MappingLookupTable::find(sm::PhysicalAddress paddr) const noexcept [[clang::nonblocking]] {
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

OsStatus MappingLookupTable::reclaimMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]] {
    for (auto it = mCache.begin(); it != mCache.end(); ++it) {
        const auto& [key, entry] = *it;
        if (entry.refs == 0) {
            *mapping = AddressMapping {
                .vaddr = entry.vaddr,
                .paddr = key.address,
                .size = entry.count * x64::kPageSize,
            };
            mCache.remove(key);
            return OsStatusSuccess;
        }
    }

    return OsStatusNotFound;
}

void MappingLookupTable::retain(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]] {
    if (auto *it = mCache.find(paddr)) {
        it->refs += 1;
    }
}

void MappingLookupTable::release(sm::PhysicalAddress paddr) noexcept [[clang::nonallocating]] {
    if (auto *it = mCache.find(paddr)) {
        if (it->refs > 0) {
            it->refs -= 1;
        }
    }
}
