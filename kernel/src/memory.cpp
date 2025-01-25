#include "memory.hpp"


km::SystemMemory::SystemMemory(const boot::MemoryMap& memmap, VirtualRange systemArea, PageBuilder pm, mem::IAllocator *alloc)
    : allocator(alloc)
    , pager(pm)
    , pmm(memmap, allocator)
    , pt(&pager, allocator)
    , vmm(systemArea, alloc)
{ }

// TODO: respect align, dont allocate such large memory ranges
void *km::SystemMemory::allocate(size_t size, size_t, PageFlags flags, MemoryType type) {
    PhysicalAddress paddr = pmm.alloc4k(pages(size));
    void *vaddr = (void*)(paddr.address + pager.hhdmOffset());
    MemoryRange range { paddr, paddr + size };
    pt.mapRange(range, vaddr, flags, type);
    return vaddr;
}

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = (uintptr_t)ptr - pager.hhdmOffset();
    MemoryRange range { start, start + size };
    pt.unmap(ptr, size);
    pmm.release(range);
}

void km::SystemMemory::unmap(void *ptr, size_t size) {
    pt.unmap(ptr, size);
}

void *km::SystemMemory::map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    MemoryRange range { begin, end };

    void *vaddr = { (void*)(begin.address + pager.hhdmOffset()) };
    pt.mapRange(range, vaddr, flags, type);
    pmm.markUsed(range);
    return vaddr;
}
