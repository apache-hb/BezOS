#include "memory/page_allocator_command_list.hpp"
#include "memory/page_allocator.hpp"

using km::PageAllocatorCommandList;

PageAllocatorCommandList::PageAllocatorCommandList(PageAllocator *allocator [[gnu::nonnull]]) noexcept
    : mList(&allocator->mMemoryHeap)
{ }

OsStatus PageAllocatorCommandList::split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo [[gnu::nonnull]], TlsfAllocation *hi [[gnu::nonnull]]) [[clang::allocating]] {
    return mList.split(allocation, midpoint, lo, hi);
}

void PageAllocatorCommandList::commit() noexcept [[clang::nonallocating]] {
    mList.commit();
}
