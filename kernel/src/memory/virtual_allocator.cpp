#include "memory/virtual_allocator.hpp"

#include "arch/paging.hpp"

km::VirtualAllocator::VirtualAllocator(VirtualRange range, mem::IAllocator *allocator)
    : mRangeAllocator(range, allocator)
{ }

km::VirtualAllocator::VirtualAllocator(mem::IAllocator *allocator)
    : mRangeAllocator(allocator)
{ }

void km::VirtualAllocator::markUsed(VirtualRange range) {
    mRangeAllocator.markUsed(range);
}

void *km::VirtualAllocator::alloc4k(size_t count) {
    size_t size = (count * x64::kPageSize);
    return (void*)mRangeAllocator.allocate(size).front;
}

void km::VirtualAllocator::release(VirtualRange range) {
    mRangeAllocator.release(range);
}
