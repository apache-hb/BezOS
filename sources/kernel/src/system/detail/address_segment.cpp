#include "system/detail/address_segment.hpp"

using sys::detail::AddressSegment;

AddressSegment::AddressSegment(km::MemoryRangeEx backingMemory, km::VmemAllocation vmemAllocation) noexcept
    : mBackingMemory(backingMemory.cast<km::PhysicalAddressEx>())
    , mVmemAllocation(vmemAllocation)
{
    KM_ASSERT(!vmemAllocation.isNull());
}

AddressSegment::AddressSegment(const AddressSegment& other) noexcept
    : AddressSegment(other.getBackingMemory(), other.getVmemAllocation())
{ }

AddressSegment& AddressSegment::operator=(const AddressSegment& other) noexcept {
    if (this != &other) {
        mBackingMemory = other.getBackingMemory();
        mVmemAllocation = other.getVmemAllocation();
    }

    return *this;
}

km::MemoryRangeEx AddressSegment::getBackingMemory() const noexcept [[clang::nonallocating]] {
    return mBackingMemory;
}

km::VmemAllocation AddressSegment::getVmemAllocation() const noexcept [[clang::nonallocating]] {
    return mVmemAllocation;
}

km::AddressMapping AddressSegment::mapping() const noexcept [[clang::nonallocating]] {
    auto front = mVmemAllocation.address();
    const void *vaddr = std::bit_cast<const void*>(front);
    return km::MappingOf(mBackingMemory, vaddr);
}

bool AddressSegment::hasBackingMemory() const noexcept [[clang::nonblocking]] {
    return !mBackingMemory.isEmpty();
}

km::VirtualRange AddressSegment::range() const noexcept [[clang::nonallocating]] {
    return mVmemAllocation.range().cast<const void*>();
}
