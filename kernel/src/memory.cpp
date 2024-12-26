#include "memory.hpp"

#include <algorithm>

km::SystemMemory::SystemMemory(SystemMemoryLayout memory, uintptr_t bits, limine_hhdm_response hhdm)
    : pager(bits, hhdm.offset)
    , layout(memory)
    , pmm(&layout)
    , vmm(&pager, &pmm)
{ }

km::VirtualAddress km::SystemMemory::hhdmMap(PhysicalAddress begin, PhysicalAddress end) {
    size_t pages = std::max((end - begin) / x64::kPageSize, 1zu);
    for (size_t i = 0; i < pages; i++) {
        hhdmMap(begin + i * x64::kPageSize);
    }

    return VirtualAddress { begin.address + pager.hhdmOffset() };
}

km::VirtualAddress km::SystemMemory::hhdmMap(PhysicalAddress paddr) {
    VirtualAddress result { paddr.address + pager.hhdmOffset() };
    vmm.map4k(paddr, result, PageFlags::eData);
    return result;
}
