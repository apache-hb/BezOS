#include "memory/virtual_allocator.hpp"

#include "arch/paging.hpp"

using VmemRange = km::AnyRange<const std::byte*>;

km::VirtualAllocator::VirtualAllocator(VirtualRange range, VirtualRange user)
    : mSupervisorRanges(VmemRange((const std::byte*)range.front, (const std::byte*)range.back))
    , mUserRanges(VmemRange((const std::byte*)user.front, (const std::byte*)user.back))
{ }

void km::VirtualAllocator::markUsed(VirtualRange range) {
    VmemRange vrange = { (const std::byte*)range.front, (const std::byte*)range.back };
    mSupervisorRanges.markUsed(vrange);
    mUserRanges.markUsed(vrange);
}

void *km::VirtualAllocator::alloc4k(size_t count, const void *hint) {
    VmemRange range = mSupervisorRanges.allocate({
        .size = (count * x64::kPageSize),
        .align = x64::kPageSize,
        .hint = (const std::byte*)hint,
    });

    if (range.isEmpty()) {
        return KM_INVALID_ADDRESS;
    }

    return (void*)range.front;
}

void *km::VirtualAllocator::alloc2m(size_t count, const void *hint) {
    VmemRange range = mSupervisorRanges.allocate({
        .size = (count * x64::kLargePageSize),
        .align = x64::kLargePageSize,
        .hint = (const std::byte*)hint,
    });

    if (range.isEmpty()) {
        return KM_INVALID_ADDRESS;
    }

    return (void*)range.front;
}

void km::VirtualAllocator::release(VirtualRange range) {
    VmemRange vrange = { (const std::byte*)range.front, (const std::byte*)range.back };
    mSupervisorRanges.release(vrange);
}

//
// User virtual memory management.
//

void *km::VirtualAllocator::userAlloc4k(size_t count, const void *hint) {
    VmemRange range = mUserRanges.allocate({
        .size = (count * x64::kPageSize),
        .align = x64::kPageSize,
        .hint = (const std::byte*)hint,
    });

    if (range.isEmpty()) {
        return KM_INVALID_ADDRESS;
    }

    return (void*)range.front;
}

void *km::VirtualAllocator::userAlloc2m(size_t count, const void *hint) {
    VmemRange range = mUserRanges.allocate({
        .size = (count * x64::kLargePageSize),
        .align = x64::kLargePageSize,
        .hint = (const std::byte*)hint,
    });

    if (range.isEmpty()) {
        return KM_INVALID_ADDRESS;
    }

    return (void*)range.front;
}

void km::VirtualAllocator::userRelease(VirtualRange range) {
    VmemRange vrange = { (const std::byte*)range.front, (const std::byte*)range.back };
    mUserRanges.release(vrange);
}
