#include "memory/virtual_allocator.hpp"

#include "arch/paging.hpp"

km::VirtualAllocator::VirtualAllocator(VirtualRange range,  VirtualRange user)
    : mSupervisorRanges(range)
    , mUserRanges(user)
{ }

void km::VirtualAllocator::markUsed(VirtualRange range) {
    mSupervisorRanges.markUsed(range);
    mUserRanges.markUsed(range);
}

void *km::VirtualAllocator::alloc4k(size_t count) {
    size_t size = (count * x64::kPageSize);
    return (void*)mSupervisorRanges.allocateAligned(size, x64::kPageSize).front;
}

void *km::VirtualAllocator::alloc2m(size_t count) {
    size_t size = (count * x64::kLargePageSize);
    return (void*)mSupervisorRanges.allocateAligned(size, x64::kLargePageSize).front;
}

void km::VirtualAllocator::release(VirtualRange range) {
    mSupervisorRanges.release(range);
}


void *km::VirtualAllocator::userAlloc4k(size_t count) {
    size_t size = (count * x64::kPageSize);
    return (void*)mUserRanges.allocateAligned(size, x64::kPageSize).front;
}

void *km::VirtualAllocator::userAlloc2m(size_t count) {
    size_t size = (count * x64::kLargePageSize);
    return (void*)mUserRanges.allocateAligned(size, x64::kLargePageSize).front;
}

void km::VirtualAllocator::userRelease(VirtualRange range) {
    mUserRanges.release(range);
}
