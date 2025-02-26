#include "memory.hpp"
#include "log.hpp"
#include "memory/memory.hpp"

km::SystemMemory::SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, PageBuilder pm, AddressMapping pteMemory)
    : pager(pm)
    , pmm(memmap)
    , ptes(pteMemory, &pager, systemArea)
{ }

void *km::SystemMemory::allocate(size_t size, PageFlags flags, MemoryType type) {
    return (void*)allocate(AllocateRequest {
        .size = size,
        .flags = flags,
        .type = type
    }).vaddr;
}

km::AddressMapping km::SystemMemory::kernelAllocate(size_t size, PageFlags flags, MemoryType type) {
    return allocate(AllocateRequest {
        .size = size,
        .flags = flags,
        .type = type
    });
}

km::AddressMapping km::SystemMemory::allocateWithHint(const void *hint, size_t size, PageFlags flags, MemoryType type) {
    return allocate(AllocateRequest {
        .size = size,
        .hint = hint,
        .flags = flags,
        .type = type
    });
}

km::AddressMapping km::SystemMemory::allocateStack(size_t size) {
    size_t pages = Pages(size);
    OsStatus status = OsStatusSuccess;

    PhysicalAddress paddr = pmm.alloc4k(pages);
    if (paddr == KM_INVALID_MEMORY) {
        return AddressMapping{};
    }

    MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };

    //
    // Allocate an extra page for the guard page
    //
    VirtualRange vmem = ptes.vmemAllocate(pages + 1);

    //
    // First reserve the memory for the stack and the guard page.
    //
    AddressMapping vmemMapping = MappingOf(vmem, 0zu);
    status = ptes.map(vmemMapping, PageFlags::eNone);
    if (status != OsStatusSuccess) {
        pmm.release(range);
        return AddressMapping{};
    }

    //
    // Then remap the usable area of stack memory.
    // The stack grows down, so the guard page is the first page in the range.
    //
    char *base = (char*)vmem.front + x64::kPageSize;
    AddressMapping mapping {
        .vaddr = base,
        .paddr = paddr,
        .size = range.size(),
    };

    status = ptes.map(mapping, PageFlags::eData, MemoryType::eWriteBack);
    if (status != OsStatusSuccess) {
        pmm.release(range);
        return AddressMapping{};
    }

    return AddressMapping {
        .vaddr = base,
        .paddr = paddr,
        .size = pages * x64::kPageSize
    };
}

km::AddressMapping km::SystemMemory::allocate(AllocateRequest request) {
    size_t pages = Pages(request.size);

    PhysicalAddress paddr = pmm.alloc4k(pages);
    if (paddr == KM_INVALID_MEMORY) {
        return AddressMapping{};
    }

    AddressMapping mapping{};
    auto range = MemoryRange::of(paddr, request.size);
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

void *km::SystemMemory::map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    //
    // I may be asked to map a range that is not page aligned
    // so I need to save the offset to apply to the virtual address before giving it back.
    //
    uintptr_t offset = (begin.address & 0xFFF);

    MemoryRange range { sm::rounddown(begin.address, x64::kPageSize), sm::roundup(end.address, x64::kPageSize) };

    reservePhysical(range);

    AddressMapping mapping{};
    OsStatus status = ptes.map(range, flags, type, &mapping);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return (void*)((uintptr_t)mapping.vaddr + offset);
}
