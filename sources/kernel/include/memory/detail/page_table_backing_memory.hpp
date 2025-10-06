#pragma once

#include "memory/detail/mapping_lookup_table.hpp"
#include "memory/table_allocator.hpp"

namespace km::detail {
    /// @brief Manages backing memory for page tables.
    ///
    /// This is responsible for allocating, tracking, and reclaiming memory used for page tables.
    /// It has a cache to track reverse lookups from virtual to physical addresses, this cache also
    /// tracks a reference count for each mapping so that memory can be reclaimed when no longer in use.
    class PageTableBackingMemory {
        MappingLookupTable mCache;
        PageTableAllocator mAllocator;

    public:
        PageTableBackingMemory() noexcept [[clang::nonallocating]] = default;

        OsStatus addBackingMemory(AddressMapping mapping) noexcept [[clang::nonallocating]];

        OsStatus reclaimBackingMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]];

        PageTableAllocation allocate() [[clang::allocating]];

        void deallocate(PageTableAllocation allocation) noexcept [[clang::nonallocating]];

        OsStatus allocateList(size_t pages, PageTableList *list [[outparam]]) [[clang::allocating]];

        OsStatus allocateExtra(size_t pages, PageTableList& list) [[clang::allocating]];

        void deallocateList(PageTableList list) noexcept [[clang::nonallocating]];

        static OsStatus create(AddressMapping pteMemory, size_t pageSize, PageTableBackingMemory *memory [[outparam]]) [[clang::allocating]];
    };
}
