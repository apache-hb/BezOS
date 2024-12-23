#include "memory/paging.hpp"

static km::PhysicalAddress VirtualToPhysical3(const km::PageManager& pm, const x64::PageMapLevel3 *pml3, km::VirtualAddress vaddr) {
    uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
    uint16_t pde = (vaddr.address >> 21) & 0b0001'1111'1111;
    uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

    const x64::pdpte *l3 = &pml3->entries[pdpte];
    if (!l3->present())
        return nullptr;

    const x64::PageMapLevel2 *pml2 = reinterpret_cast<const x64::PageMapLevel2*>(pm.address(*l3));
    const x64::pde *l2 = &pml2->entries[pde];
    if (!l2->present())
        return nullptr;

    const x64::PageTable *pml1 = reinterpret_cast<const x64::PageTable*>(pm.address(*l2));
    const x64::pte *l1 = &pml1->entries[pte];
    if (!l1->present())
        return nullptr;

    return km::PhysicalAddress { pm.address(*l1) };
}

km::PhysicalAddress km::PageManager::translate(km::VirtualAddress vaddr) const {
    const void *pagemap = activeMap().as<void>(); // TODO: wrong

    uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;

    const x64::PageMapLevel4 *l4 = reinterpret_cast<const x64::PageMapLevel4*>(pagemap);

    const x64::pml4e *t4 = &l4->entries[pml4e];
    if (!t4->present())
        return nullptr;

    return VirtualToPhysical3(*this, (const x64::PageMapLevel3*)pagemap, vaddr);
}
