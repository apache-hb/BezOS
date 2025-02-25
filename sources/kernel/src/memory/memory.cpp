#include "memory/memory.hpp"

km::PageTables::PageTables(km::AddressMapping pteMemory, const km::PageBuilder *pm)
    : mSlide(pteMemory.slide())
    , mAllocator((void*)pteMemory.vaddr, pteMemory.size)
    , mPageManager(pm)
    , mRootPageTable((x64::PageMapLevel4*)allocPage())
{ }

x64::page *km::PageTables::allocPage() {
    return mAllocator.construct<x64::page>(x64::page{});
}

OsStatus km::PageTables::map(MappingRequest, AddressMapping*) {
    return OsStatusSuccess;
}

OsStatus km::PageTables::unmap(VirtualRange range) noexcept {
    KM_CHECK((uintptr_t)range.front % x64::kPageSize == 0, "Virtual address must be page aligned.");
    KM_CHECK((uintptr_t)range.back % x64::kPageSize == 0, "Virtual address must be page aligned.");

    x64::PageMapLevel4 *l4 = mRootPageTable;

    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back; i += x64::kPageSize) {
        uint16_t pml4e = (i >> 39) & 0b0001'1111'1111;
        uint16_t pdpte = (i >> 30) & 0b0001'1111'1111;
        uint16_t pdte = (i >> 21) & 0b0001'1111'1111;
        uint16_t pte = (i >> 12) & 0b0001'1111'1111;

        const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
        if (!l3) continue;

        const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
        if (!l2) continue;

        x64::PageTable *pt = findPageTable(l2, pdte);
        if (!pt) continue;

        x64::pte& t1 = pt->entries[pte];
        t1.setPresent(false);
        __invlpg(i);
    }

    return OsStatusSuccess;
}

OsStatus km::PageTables::map4k(const void *vaddr, PhysicalAddress paddr, PageFlags flags, MemoryType type, PageFlags middle) {
    KM_CHECK((uintptr_t)vaddr % x64::kPageSize == 0, "Virtual address must be 4k aligned.");
    KM_CHECK(paddr.address % x64::kPageSize == 0, "Physical address must be 4k aligned.");

    uintptr_t addr = (uintptr_t)vaddr;
    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (addr >> 21) & 0b0001'1111'1111;
    uint16_t pte = (addr >> 12) & 0b0001'1111'1111;

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel3 *l3 = getPageMap3(mRootPageTable, pml4e, middle);
    if (l3 == nullptr) {
        return OsStatusOutOfMemory;
    }

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, middle);
    if (l2 == nullptr) {
        return OsStatusOutOfMemory;
    }

    x64::PageTable *pt = getPageTable(l2, pdte, middle);
    if (pt == nullptr) {
        return OsStatusOutOfMemory;
    }

    x64::pte& t1 = pt->entries[pte];
    mPageManager->setMemoryType(t1, type);
    setEntryFlags(t1, flags, paddr, true);

    return OsStatusSuccess;
}

OsStatus km::PageTables::map2m(const void *vaddr, PhysicalAddress paddr, PageFlags flags, MemoryType type, PageFlags middle) {
    KM_CHECK((uintptr_t)vaddr % x64::kLargePageSize == 0, "Virtual address must be 2m aligned.");
    KM_CHECK(paddr.address % x64::kLargePageSize == 0, "Physical address must be 2m aligned.");

    uintptr_t addr = (uintptr_t)vaddr;
    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (addr >> 21) & 0b0001'1111'1111;

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel3 *l3 = getPageMap3(mRootPageTable, pml4e, middle);
    if (l3 == nullptr) {
        return OsStatusOutOfMemory;
    }

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, middle);
    if (l2 == nullptr) {
        return OsStatusOutOfMemory;
    }

    x64::pde& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr, true);

    return OsStatusSuccess;
}

void km::PageTables::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address, bool present) noexcept [[clang::nonblocking]] {
    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));
    entry.setUser(bool(flags & PageFlags::eUser));

    entry.setPresent(present);
}

x64::PageMapLevel3 *km::PageTables::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(allocPage());
        if (l3 != nullptr) {
            setEntryFlags(t4, flags, asPhysical(l3), true);
        }
    } else {
        l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *km::PageTables::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(allocPage());
        if (l3 != nullptr) {
            setEntryFlags(t3, flags, asPhysical(l2), true);
        }
    } else {
        l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    }

    return l2;
}

x64::PageTable *km::PageTables::getPageTable(x64::PageMapLevel2 *l2, uint16_t pdte, PageFlags flags) {
    x64::PageTable *pt;

    x64::pde& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(allocPage());
        if (pt != nullptr) {
            setEntryFlags(t2, flags, asPhysical(pt), true);
        }
    } else {
        pt = asVirtual<x64::PageTable>(mPageManager->address(t2));
    }

    return pt;
}

const x64::PageMapLevel3 *km::PageTables::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const noexcept [[clang::nonblocking]] {
    const x64::pml4e& t4 = l4->entries[pml4e];
    if (t4.present()) {
        uintptr_t base = mPageManager->address(t4);
        return asVirtual<x64::PageMapLevel3>(base);
    }

    return (x64::PageMapLevel3*)KM_INVALID_ADDRESS;
}

const x64::PageMapLevel2 *km::PageTables::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const noexcept [[clang::nonblocking]] {
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (t3.present() && !t3.is1g()) {
        uintptr_t base = mPageManager->address(t3);
        return asVirtual<x64::PageMapLevel2>(base);
    }

    return (x64::PageMapLevel2*)KM_INVALID_ADDRESS;
}

x64::PageTable *km::PageTables::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const noexcept [[clang::nonblocking]] {
    const x64::pde& t2 = l2->entries[pdte];
    if (t2.present() && !t2.is2m()) {
        uintptr_t base = mPageManager->address(t2);
        return asVirtual<x64::PageTable>(base);
    }

    return (x64::PageTable*)KM_INVALID_ADDRESS;
}

km::PhysicalAddress km::PageTables::asPhysical(const void *ptr) const noexcept [[clang::nonblocking]] {
    return PhysicalAddress { (uintptr_t)ptr - mSlide };
}
