#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, uintptr_t bits, uintptr_t hhdmOffset, PageMemoryTypeLayout types)
    : pager(bits, hhdmOffset, types)
    , layout(memory)
    , pmm(&layout, hhdmOffset)
    , vmm(&pager, &pmm)
{ }

void km::SystemMemory::unmap(void *ptr, size_t size) {
    vmm.unmap(ptr, size);
}


void *km::SystemMemory::hhdmMap(PhysicalAddress begin, PhysicalAddress end, PageFlags flags) {
    MemoryRange range { begin, end };
    VirtualAddress vaddr = { begin.address + pager.hhdmOffset() };
    vmm.mapRange(range, vaddr, flags);
    pmm.markRangeUsed(range);
    return (void*)vaddr.address;
}
