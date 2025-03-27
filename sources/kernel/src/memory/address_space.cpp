#include "memory/address_space.hpp"

km::AddressSpace::AddressSpace(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRange vmem)
    : mTables(pm, pteMemory, flags)
    , mVmemAllocator(vmem)
{ }

OsStatus km::AddressSpace::unmap(VirtualRange range) {
    VirtualRange aligned = alignedOut(range, x64::kPageSize);

    //
    // If unmapping page tables fails then we must not release the virtual address
    // space associated with it. Unmap failing has a strong gaurantee that no
    // page tables were modified.
    //
    OsStatus status = mTables.unmap(aligned);
    if (status == OsStatusSuccess) {
        mVmemAllocator.release(aligned);
    }

    return status;
}

OsStatus km::AddressSpace::unmap(void *ptr, size_t size) {
    return unmap(VirtualRange::of(ptr, size));
}

void *km::AddressSpace::map(MemoryRange range, PageFlags flags, MemoryType type) {
    uintptr_t offset = range.front.address % x64::kPageSize;

    size_t pages = Pages(range.size());
    VirtualRange vmem = mVmemAllocator.allocate(pages);
    if (vmem.isEmpty()) {
        return nullptr;
    }

    AddressMapping m = MappingOf(vmem, range.front);
    OsStatus status = mTables.map(m, flags, type);
    if (status != OsStatusSuccess) {
        mVmemAllocator.release(vmem);
        return nullptr;
    }

    return (void*)((uintptr_t)vmem.front + offset);
}
