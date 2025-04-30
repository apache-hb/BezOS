#include "memory/address_space.hpp"
#include "memory/tables.hpp"
#include "memory/allocator.hpp"

km::AddressSpace::AddressSpace(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRange vmem)
    : mTables(pm, pteMemory, flags)
    , mVmemAllocator(vmem)
{ }

km::AddressSpace::AddressSpace(const AddressSpace *source, AddressMapping pteMemory, PageFlags flags, VirtualRange vmem)
    : mTables(source->pageManager(), pteMemory, flags)
    , mVmemAllocator(vmem)
{
    updateHigherHalfMappings(source);
}

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
    MemoryRange aligned = alignedOut(range, x64::kPageSize);

    size_t pages = Pages(aligned.size());
    VirtualRange vmem = mVmemAllocator.allocate(pages);
    if (vmem.isEmpty()) {
        return nullptr;
    }

    AddressMapping m = MappingOf(vmem, aligned.front);
    OsStatus status = mTables.map(m, flags, type);
    if (status != OsStatusSuccess) {
        mVmemAllocator.release(vmem);
        return nullptr;
    }

    return (void*)((uintptr_t)vmem.front + offset);
}

OsStatus km::AddressSpace::map(MemoryRange range, const void *hint, PageFlags flags, MemoryType type, AddressMapping *mapping) {
    size_t pages = Pages(range.size());
    VirtualRange vmem = mVmemAllocator.allocate(pages, hint);
    if (vmem.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    AddressMapping m = MappingOf(vmem, range.front);
    if (OsStatus status = mTables.map(m, flags, type)) {
        mVmemAllocator.release(vmem);
        return status;
    }

    *mapping = m;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::mapStack(MemoryRange range, PageFlags flags, StackMapping *mapping) {
    size_t pages = Pages(range.size());
    VirtualRange total = mVmemAllocator.allocate(pages + 2);
    if (total.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    AddressMapping stack = MappingOf(range, (char*)total.front + x64::kPageSize);
    if (OsStatus status = mTables.map(stack, flags, MemoryType::eWriteBack)) {
        mVmemAllocator.release(total);
        return status;
    }

    *mapping = StackMapping { stack, total };

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::unmapStack(StackMapping mapping) {
    if (OsStatus status = unmap(mapping.stack.virtualRange())) {
        return status;
    }

    mVmemAllocator.release(mapping.total);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::reserve(AddressMapping mapping, PageFlags flags, MemoryType type) {
    mVmemAllocator.reserve(mapping.virtualRange());

    if (OsStatus status = mTables.map(mapping, flags, type)) {
        mVmemAllocator.release(mapping.virtualRange());
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::reserve(size_t size, VirtualRange *result) {
    VirtualRange range = mVmemAllocator.allocate(km::Pages(size));
    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    *result = range;
    return OsStatusSuccess;
}

void km::AddressSpace::updateHigherHalfMappings(const PageTables *source) {
    km::copyHigherHalfMappings(&mTables, source);
}

OsStatus km::AddressSpace::map(MemoryRangeEx memory, PageFlags flags, MemoryType type, TlsfAllocation *allocation) {
    if (memory.isEmpty() || (memory != alignedOut(memory, x64::kPageSize))) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    TlsfAllocation vmem = mVmemHeap.aligned_alloc(x64::kPageSize, memory.size());
    if (vmem.isNull()) {
        return OsStatusOutOfMemory;
    }

    AddressMapping m = {
        .vaddr = std::bit_cast<const void*>(vmem.address()),
        .paddr = memory.front.address,
        .size = memory.size()
    };

    if (OsStatus status = mTables.map(m, flags, type)) {
        mVmemHeap.free(vmem);
        return status;
    }

    *allocation = vmem;
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::map(TlsfAllocation memory, PageFlags flags, MemoryType type, MappingAllocation *allocation) {
    TlsfAllocation result;

    if (OsStatus status = map(memory.range().cast<km::PhysicalAddressEx>(), flags, type, &result)) {
        return status;
    }

    *allocation = MappingAllocation::unchecked(memory, result);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::unmap(MappingAllocation allocation) {
    return unmap(allocation.virtualAllocation());
}

OsStatus km::AddressSpace::unmap(TlsfAllocation allocation) {
    if (OsStatus status = mTables.unmap(allocation.range().cast<const void*>())) {
        return status;
    }

    stdx::LockGuard guard(mLock);
    mVmemHeap.free(allocation);
    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space) {
    if (OsStatus status = km::PageTables::create(pm, pteMemory, flags, &space->mTables)) {
        return status;
    }

    if (OsStatus status = km::TlsfHeap::create(vmem.cast<km::PhysicalAddress>(), &space->mVmemHeap)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus km::AddressSpace::create(const AddressSpace *source, AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space) {
    if (OsStatus status = km::AddressSpace::create(source->pageManager(), pteMemory, flags, vmem, space)) {
        return status;
    }

    space->updateHigherHalfMappings(source);
    return OsStatusSuccess;
}
