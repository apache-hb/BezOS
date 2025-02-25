#include "memory.hpp"
#include "log.hpp"
#include "memory/memory.hpp"

km::SystemMemory::SystemMemory(std::span<const boot::MemoryRegion> memmap, VirtualRange systemArea, VirtualRange userArea, PageBuilder pm, AddressMapping pteMemory)
    : pager(pm)
    , pmm(memmap)
    , pt(&pager, pteMemory)
    , vmm(systemArea, userArea)
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

km::AddressMapping km::SystemMemory::userAllocate(size_t size, PageFlags flags, MemoryType type) {
    size_t pages = Pages(size);

    PhysicalAddress paddr = pmm.alloc4k(pages);
    MemoryRange range { paddr, paddr + size };
    if (paddr == KM_INVALID_MEMORY) {
        return AddressMapping{};
    }

    void *vaddr = vmm.userAlloc4k(pages);
    if (vaddr == KM_INVALID_ADDRESS) {
        pmm.release(range);
        return AddressMapping{};
    }

    pt.mapRange(range, vaddr, flags, type);

    return AddressMapping {
        .vaddr = vaddr,
        .paddr = paddr,
        .size = pages * x64::kPageSize
    };
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

    PhysicalAddress paddr = pmm.alloc4k(pages);
    if (paddr == KM_INVALID_MEMORY) {
        return AddressMapping{};
    }

    //
    // Allocate an extra page for the guard page
    //
    void *vaddr = vmm.alloc4k(pages + 1);
    MemoryRange range { paddr, paddr + (pages * x64::kPageSize) };
    if (vaddr == KM_INVALID_ADDRESS) {
        pmm.release(range);
        return AddressMapping{};
    }

    //
    // The stack grows down, so the guard page is at the top of the stack.
    //
    char *base = (char*)vaddr + x64::kPageSize;

    pt.mapRange(range, base, PageFlags::eData, MemoryType::eWriteBack);

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

    void *vaddr = vmm.alloc4k(pages, request.hint);
    MemoryRange range { paddr, paddr + request.size };
    if (vaddr == KM_INVALID_ADDRESS) {
        pmm.release(range);
        return AddressMapping{};
    }

    pt.mapRange(range, vaddr, request.flags, request.type);

    return AddressMapping {
        .vaddr = vaddr,
        .paddr = paddr,
        .size = pages * x64::kPageSize
    };
}

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = pt.getBackingAddress(ptr);
    if (start == KM_INVALID_MEMORY) {
        KmDebugMessage("[WARN] Attempted to release ", ptr, " but it is not mapped.\n");
        return;
    }

    MemoryRange range { start, start + size };
    pt.unmap(ptr, size);
    pmm.release(range);
}

void km::SystemMemory::unmap(void *ptr, size_t size) {
    pt.unmap(ptr, size);
}

void *km::SystemMemory::map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    //
    // I may be asked to map a range that is not page aligned
    // so I need to save the offset to apply to the virtual address before giving it back.
    //
    uintptr_t offset = (begin.address & 0xFFF);

    MemoryRange range { begin, end };

    //
    // If the range falls on a 2m boundary and is at least 2m in size, use a large page
    // to save on page table entries.
    //
    void *vaddr = [&] {
        if (begin.address % x64::kLargePageSize == 0 && range.size() >= x64::kLargePageSize) {
            return vmm.alloc2m(LargePages(range.size()));
        }

        return vmm.alloc4k(Pages(range.size()));
    }();

    pt.mapRange(range, vaddr, flags, type);
    pmm.markUsed(range);

    // Apply the offset to the virtual address
    return (void*)((uintptr_t)vaddr + offset);
}
