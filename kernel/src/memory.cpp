#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, uintptr_t bits, uintptr_t hhdmOffset, PageMemoryTypeLayout types)
    : pager(bits, hhdmOffset, types)
    , layout(memory)
    , pmm(&layout, hhdmOffset)
    , vmm(&pager, &pmm)
{ }

// TODO: respect align, dont allocate such large memory ranges
void *km::SystemMemory::allocate(size_t size, size_t) {
    PhysicalAddress paddr = pmm.alloc4k(pages(size));
    VirtualAddress vaddr = { paddr.address + pager.hhdmOffset() };
    MemoryRange range { paddr, paddr + size };
    vmm.mapRange(range, vaddr, PageFlags::eData);
    return (void*)vaddr.address;
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

void *km::SystemMemory::hhdmMap(PhysicalAddress begin, PhysicalAddress end, PageFlags flags, MemoryType type) {
    MemoryRange range { begin, end };
    VirtualAddress vaddr = { begin.address + pager.hhdmOffset() };
    vmm.mapRange(range, vaddr, flags, type);
    pmm.markRangeUsed(range);
    return (void*)vaddr.address;
}
