#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, PageBuilder pm, mem::IAllocator *allocator)
    : pager(pm)
    , layout(memory)
    , pmm(&layout, allocator)
    , vmm(&pager, allocator)
{ }

// TODO: respect align, dont allocate such large memory ranges
void *km::SystemMemory::allocate(size_t size, size_t, PageFlags flags, MemoryType type) {
    PhysicalAddress paddr = pmm.alloc4k(pages(size));
    void *vaddr = (void*)(paddr.address + pager.hhdmOffset());
    MemoryRange range { paddr, paddr + size };
    vmm.mapRange(range, vaddr, flags, type);
    return vaddr;
}

void km::SystemMemory::release(void *ptr, size_t size) {
    PhysicalAddress start = (uintptr_t)ptr - pager.hhdmOffset();
    MemoryRange range { start, start + size };
    vmm.unmap(ptr, size);
    pmm.release(range);
}

void km::SystemMemory::unmap(void *ptr, size_t size) {
    vmm.unmap(ptr, size);
}

void *km::SystemMemory::map(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    MemoryRange range { begin, end };

    void *vaddr = { (void*)(begin.address + pager.hhdmOffset()) };
    vmm.mapRange(range, vaddr, flags, type);
    pmm.markUsed(range);
    return vaddr;
}
