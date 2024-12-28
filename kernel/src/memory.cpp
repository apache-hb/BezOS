#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, uintptr_t bits, uintptr_t hhdmOffset, PageMemoryTypeLayout types)
    : pager(bits, hhdmOffset, types)
    , layout(memory)
    , pmm(&layout)
    , vmm(&pager, &pmm)
{ }

km::VirtualAddress km::SystemMemory::hhdmMap(PhysicalAddress begin, PhysicalAddress end) {
    MemoryRange range { begin, end };
    VirtualAddress vaddr = { begin.address + pager.hhdmOffset() };
    vmm.mapRange(range, vaddr, PageFlags::eData);
    return vaddr;
}
