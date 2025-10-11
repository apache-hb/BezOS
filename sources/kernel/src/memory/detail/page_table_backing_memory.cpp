#include "memory/detail/page_table_backing_memory.hpp"

using km::detail::PageTableBackingMemory;
using km::PageTableAllocation;

OsStatus PageTableBackingMemory::addMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    return mCache.addMemory(mapping);
}

OsStatus PageTableBackingMemory::reclaimMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]] {
    return mCache.reclaimMemory(mapping);
}

PageTableAllocation PageTableBackingMemory::allocate() [[clang::allocating]] {
    PageTableAllocation allocation = mAllocator.allocate(1);
    if (!allocation.isPresent()) {
        return {};
    }

    mCache.retain(allocation.getPhysical());
    return allocation;
}

void PageTableBackingMemory::deallocate(PageTableAllocation allocation) noexcept [[clang::nonallocating]] {
    mAllocator.deallocate(allocation.getVirtual(), 1, allocation.getSlide());
    mCache.release(allocation.getPhysical());
}
