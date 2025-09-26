#include "memory/page_allocator_command_list.hpp"
#include "memory/page_allocator.hpp"

using km::PageAllocatorCommandList;

PageAllocatorCommandList::PageAllocatorCommandList(PageAllocator *allocator [[gnu::nonnull]]) noexcept
    : mList(&allocator->mMemoryHeap)
{ }

OsStatus PageAllocatorCommandList::split(km::PmmAllocation allocation, PhysicalAddress midpoint, km::PmmAllocation *lo [[outparam]], km::PmmAllocation *hi [[outparam]]) [[clang::allocating]] {
    return mList.split(allocation, midpoint, lo, hi);
}

void PageAllocatorCommandList::commit() noexcept [[clang::nonallocating]] {
    mList.commit();
}
