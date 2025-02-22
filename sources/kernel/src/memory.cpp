#include "memory.hpp"
#include "log.hpp"

km::SystemMemory::SystemMemory(const boot::MemoryMap& memmap, VirtualRange systemArea, VirtualRange userArea, PageBuilder pm, mem::IAllocator *alloc)
    : allocator(alloc)
    , pager(pm)
    , pmm(memmap)
    , pt(&pager, allocator)
    , vmm(systemArea, userArea)
{ }

void *km::SystemMemory::allocate(size_t size, PageFlags flags, MemoryType type) {
    PhysicalAddress paddr = pmm.alloc4k(Pages(size));
    void *vaddr = vmm.alloc4k(Pages(size));
    MemoryRange range { paddr, paddr + size };
    pt.mapRange(range, vaddr, flags, type);
    return vaddr;
}

km::AddressMapping km::SystemMemory::kernelAllocate(size_t size, PageFlags flags, MemoryType type) {
    size_t pages = Pages(size);
    PhysicalAddress paddr = pmm.alloc4k(Pages(size));
    void *vaddr = vmm.alloc4k(Pages(size));
    MemoryRange range { paddr, paddr + size };
    pt.mapRange(range, vaddr, flags, type);

    return AddressMapping {
        .vaddr = vaddr,
        .paddr = paddr,
        .size = pages * x64::kPageSize
    };
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
    size_t pages = Pages(size);

    PhysicalAddress paddr = pmm.alloc4k(pages);
    if (paddr == KM_INVALID_MEMORY) {
        return AddressMapping{};
    }

    void *vaddr = vmm.userAlloc4k(pages, hint);
    MemoryRange range { paddr, paddr + size };
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

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = pt.getBackingAddress(ptr);
    if (start == nullptr) {
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
