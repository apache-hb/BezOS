#include "memory.hpp"
#include "log.hpp"
#include "memory/memory.hpp"

km::SystemMemory::SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory)
    : pager(pm)
    , pmm(memmap)
    , ptes(&pager, pteMemory, PageFlags::eAll, systemArea)
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

    MemoryRange range = pmm.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    StackMapping mapping;
    if (ptes.mapStack(range, PageFlags::eData, &mapping) != OsStatusSuccess) {
        pmm.release(range);
        return AddressMapping{};
    }

    return mapping.mapping;
}

km::AddressMapping km::SystemMemory::allocate(AllocateRequest request) {
    size_t pages = Pages(request.size);

    MemoryRange range = pmm.alloc4k(pages);
    if (range.isEmpty()) {
        return AddressMapping{};
    }

    AddressMapping mapping{};
    OsStatus status = ptes.map(range, request.flags, request.type, &mapping);
    if (status != OsStatusSuccess) {
        pmm.release(range);
        return AddressMapping{};
    }

    return mapping;
}

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = systemTables().getBackingAddress(ptr);
    if (start == KM_INVALID_MEMORY) {
        KmDebugMessage("[WARN] Attempted to release ", ptr, " but it is not mapped.\n");
        return;
    }

    ptes.unmap(VirtualRange::of(ptr, size));
    pmm.release(MemoryRange::of(start, size));
}

void km::SystemMemory::unmap(void *ptr, size_t size) {
    ptes.unmap(VirtualRange::of(ptr, size));
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
    OsStatus status = ptes.map(aligned, flags, type, &mapping);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return (void*)((uintptr_t)mapping.vaddr + offset);
}
