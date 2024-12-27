#include "memory.hpp"

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, uintptr_t bits, limine_hhdm_response hhdm)
    : pager(bits, hhdm.offset)
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
