#include "memory.hpp"

#include "memory/memory.hpp"

km::SystemMemory::SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory)
    : mPageManager(pm)
    , mPageAllocator(memmap)
    , mTables(&mPageManager, pteMemory, PageFlags::eAll, systemArea)
{ }

void *km::SystemMemory::allocate(size_t size, PageFlags flags, MemoryType type) {
    return (void*)allocate(AllocateRequest {
        .size = size,
        .flags = flags,
        .type = type
    }).vaddr;
}

km::AddressMapping km::SystemMemory::allocateStack(size_t size) {
    size_t pages = Pages(size);

    MemoryRange range = mPageAllocator.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    StackMapping mapping;
    if (mTables.mapStack(range, PageFlags::eData, &mapping) != OsStatusSuccess) {
        mPageAllocator.release(range);
        return AddressMapping{};
    }

    return mapping.stack;
}

km::AddressMapping km::SystemMemory::allocate(AllocateRequest request) {
    size_t pages = Pages(request.size);

    MemoryRange range = mPageAllocator.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    AddressMapping mapping{};
    OsStatus status = mTables.map(range, request.flags, request.type, &mapping);
    if (status != OsStatusSuccess) {
        mPageAllocator.release(range);
        return AddressMapping{};
    }

    return mapping;
}

OsStatus km::SystemMemory::unmap(void *ptr, size_t size) {
    return unmap(VirtualRange::of(ptr, size));
}

OsStatus km::SystemMemory::unmap(AddressMapping mapping) {
    if (OsStatus status = mTables.unmap(mapping.virtualRange())) {
        return status;
    }

    mPageAllocator.release(mapping.physicalRange());
    return OsStatusSuccess;
}

void *km::SystemMemory::map(MemoryRange range, PageFlags flags, MemoryType type) {
    //
    // I may be asked to map a range that is not page aligned
    // so I need to save the offset to apply to the virtual address before giving it back.
    //
    uintptr_t offset = (range.front.address & 0xFFF);

    MemoryRange aligned = PageAligned(range);

    reservePhysical(aligned);

    AddressMapping mapping{};
    OsStatus status = mTables.map(aligned, flags, type, &mapping);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return (void*)((uintptr_t)mapping.vaddr + offset);
}

OsStatus km::SystemMemory::map(size_t size, PageFlags flags, MemoryType type, AddressMapping *mapping) {
    MemoryRange pmm = mPageAllocator.alloc4k(Pages(size));
    if (pmm.isEmpty()) {
        return OsStatusOutOfMemory;
    }
    OsStatus status = mTables.map(pmm, flags, type, mapping);
    if (status != OsStatusSuccess) {
        mPageAllocator.release(pmm);
        return status;
    }

    return OsStatusSuccess;
}
