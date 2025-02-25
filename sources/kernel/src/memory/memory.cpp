#include "memory/memory.hpp"
#include "log.hpp"

using PageTables = km::PageTables;

km::PageTables::PageTables(km::AddressMapping pteMemory, const km::PageBuilder *pm)
    : mSlide(pteMemory.slide())
    , mAllocator((void*)pteMemory.vaddr, pteMemory.size)
    , mPageManager(pm)
    , mRootPageTable((x64::PageMapLevel4*)allocPage())
{ }

x64::page *PageTables::allocPage() {
    if (x64::page *page = mAllocator.construct<x64::page>()) {
        memset(page, 0, sizeof(x64::page));

        return page;
    }

    KM_ASSERT("Failed to allocate page table.");

    return nullptr;
}

void PageTables::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
    if (address > mPageManager->maxPhysicalAddress()) {
        KmDebugMessage("Physical address out of range: ", address, " > ", mPageManager->maxPhysicalAddress(), "\n");
        KM_PANIC("Physical address out of range.");
    }

    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));
    entry.setUser(bool(flags & PageFlags::eUser));

    entry.setPresent(true);
}

x64::PageMapLevel3 *PageTables::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(allocPage());
        if (l3) setEntryFlags(t4, flags, asPhysical(l3));
    } else {
        l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *PageTables::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(allocPage());
        if (l2) setEntryFlags(t3, flags, asPhysical(l2));
    } else {
        l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    }

    return l2;
}

const x64::PageMapLevel3 *PageTables::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return asVirtual<x64::PageMapLevel3>(base);
    }

    return nullptr;
}

const x64::PageMapLevel2 *PageTables::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const {
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (t3.present() && !t3.is1g()) {
        uintptr_t base = mPageManager->address(t3);
        return asVirtual<x64::PageMapLevel2>(base);
    }

    return nullptr;
}

x64::PageTable *PageTables::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const {
    const x64::pde& pde = l2->entries[pdte];
    if (pde.present() && !pde.is2m()) {
        uintptr_t base = mPageManager->address(pde);
        return asVirtual<x64::PageTable>(base);
    }

    return nullptr;
}

OsStatus PageTables::mapRange4k(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
        if (OsStatus status = map4k(i, vaddr, flags, type)) {
            return status;
        }

        vaddr = (char*)vaddr + x64::kPageSize;
    }

    return OsStatusSuccess;
}

OsStatus PageTables::mapRange2m(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kLargePageSize) {
        if (OsStatus status = map2m(i, vaddr, flags, type)) {
            return status;
        }

        vaddr = (char*)vaddr + x64::kLargePageSize;
    }

    return OsStatusSuccess;
}

OsStatus PageTables::mapRange1g(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kHugePageSize) {
        if (OsStatus status = map1g(i, vaddr, flags, type)) {
            return status;
        }

        vaddr = (char*)vaddr + x64::kHugePageSize;
    }

    return OsStatusSuccess;
}

OsStatus PageTables::map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    stdx::LockGuard guard(mLock);

    uintptr_t addr = (uintptr_t)vaddr;

    if (!mPageManager->isCanonicalAddress(vaddr)) {
        KmDebugMessage("Attempting to map address that isn't canonical: ", Hex(addr).pad(16), "\n");
        KM_PANIC("Invalid memory mapping.");
    }

    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (addr >> 21) & 0b0001'1111'1111;
    uint16_t pte = (addr >> 12) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eAllUser);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, PageFlags::eAllUser);
    if (!l2) return OsStatusOutOfMemory;

    x64::PageTable *pt;

    x64::pde& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(allocPage());
        if (!pt) return OsStatusOutOfMemory;

        setEntryFlags(t2, PageFlags::eAllUser, asPhysical(pt));
    } else {
        pt = asVirtual<x64::PageTable>(mPageManager->address(t2));
    }

    x64::pte& t1 = pt->entries[pte];
    mPageManager->setMemoryType(t1, type);
    setEntryFlags(t1, flags, paddr);

    return OsStatusSuccess;
}

OsStatus PageTables::map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    stdx::LockGuard guard(mLock);

    uintptr_t addr = (uintptr_t)vaddr;

    if (!mPageManager->isCanonicalAddress(vaddr)) {
        KmDebugMessage("Attempting to map address that isn't canonical: ", Hex(addr).pad(16), "\n");
        KM_PANIC("Invalid memory mapping.");
    }

    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (addr >> 21) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eAllUser);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, PageFlags::eAllUser);
    if (!l2) return OsStatusOutOfMemory;

    x64::pde& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr);

    return OsStatusSuccess;
}

OsStatus PageTables::map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    stdx::LockGuard guard(mLock);

    uintptr_t addr = (uintptr_t)vaddr;
    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eAllUser);
    if (!l3) return OsStatusOutOfMemory;

    x64::pdpte& t3 = l3->entries[pdpte];
    t3.set1g(true);
    mPageManager->setMemoryType(t3, type);
    setEntryFlags(t3, flags, paddr);

    return OsStatusSuccess;
}

static bool IsLargePageAligned(km::PhysicalAddress paddr, const void *vaddr) {
    uintptr_t addr = (uintptr_t)vaddr;
    return (paddr.address & (x64::kLargePageSize - 1)) == 0 && (addr & (x64::kLargePageSize - 1)) == 0;
}

OsStatus PageTables::mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    KM_CHECK(range.size() % x64::kPageSize == 0, "Range size must be page aligned.");
    KM_CHECK((uintptr_t)vaddr % x64::kPageSize == 0, "Virtual address must be page aligned.");
    KM_CHECK(range.front.address % x64::kPageSize == 0, "Physical address must be page aligned.");

    //
    // We can use large pages if the range is larger than 2m and both the physical and virtual
    // addresses are aligned to 2m boundaries.
    // TODO: This check could be less strict, and only check that the alignment is equal
    //       relative to the large page size.
    //
    if ((uintptr_t)range.size() >= x64::kLargePageSize && IsLargePageAligned(range.front, vaddr)) {
        //
        // If the range is larger than 2m in total, check if
        // the range is still larger than 2m after aligning the range.
        //
        PhysicalAddress front2m = sm::roundup(range.front.address, x64::kLargePageSize);
        PhysicalAddress back2m = sm::rounddown(range.back.address, x64::kLargePageSize);

        //
        // If the range is still larger than 2m, we can use 2m pages.
        //
        if (front2m < back2m) {
            //
            // Map the leading 4k pages we need to map to fulfill our api contract.
            //
            MemoryRange head = {range.front, front2m};
            if (!head.isEmpty())
                mapRange4k(head, vaddr, flags, type);

            //
            // Then map the middle 2m pages.
            //
            MemoryRange body = {front2m, back2m};
            mapRange2m(body, (char*)vaddr + head.size(), flags, type);

            //
            // Finally map the trailing 4k pages.
            //
            MemoryRange tail = {back2m, range.back};
            if (!tail.isEmpty())
                mapRange4k(tail, (char*)vaddr + head.size() + body.size(), flags, type);

            return OsStatusSuccess;
        }
    }

    //
    // If we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages.
    //
    mapRange4k(range, vaddr, flags, type);

    return OsStatusSuccess;
}

void PageTables::unmap(void *ptr, size_t size) {
    stdx::LockGuard guard(mLock);

    uintptr_t front = sm::rounddown((uintptr_t)ptr, x64::kPageSize);
    uintptr_t back = sm::roundup((uintptr_t)ptr + size, x64::kPageSize);

    x64::PageMapLevel4 *l4 = getRootTable();

    for (uintptr_t i = front; i < back; i += x64::kPageSize) {
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
        x64::invlpg(i);
    }
}

km::PhysicalAddress PageTables::getBackingAddress(const void *ptr) const {
    stdx::LockGuard guard(mLock);

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;
    uintptr_t offset = address & 0xFFF;

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return KM_INVALID_MEMORY;

    if (l3->entries[pdpte].is1g()) {
        uintptr_t pdpteOffset = address & 0x3FF'000;
        return km::PhysicalAddress { mPageManager->address(l3->entries[pdpte]) + pdpteOffset + offset };
    }

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return KM_INVALID_MEMORY;

    if (l2->entries[pdte].is2m()) {
        uintptr_t pdteOffset = address & 0x1FF'000;
        return km::PhysicalAddress { mPageManager->address(l2->entries[pdte]) + pdteOffset + offset };
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return KM_INVALID_MEMORY;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return KM_INVALID_MEMORY;

    return km::PhysicalAddress { mPageManager->address(t1) + offset };
}

km::PageFlags PageTables::getMemoryFlags(const void *ptr) const {
    stdx::LockGuard guard(mLock);

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;

    PageFlags flags = PageFlags::eAllUser;

    auto applyFlags = [&](auto entry) {
        if (!entry.writeable()) flags &= ~PageFlags::eWrite;
        if (!entry.executable()) flags &= ~PageFlags::eExecute;
        if (!entry.user()) flags &= ~PageFlags::eUser;
    };

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return PageFlags::eNone;

    applyFlags(l3->entries[pdpte]);

    if (l3->entries[pdpte].is1g()) {
        return flags;
    }

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return PageFlags::eNone;

    applyFlags(l2->entries[pdte]);

    if (l2->entries[pdte].is2m()) {
        return flags;
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return PageFlags::eNone;

    applyFlags(pt->entries[pte]);

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return PageFlags::eNone;

    applyFlags(t1);

    return flags;
}

km::PageSize2 PageTables::getPageSize(const void *ptr) const {
    stdx::LockGuard guard(mLock);

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return PageSize2::eNone;

    if (l3->entries[pdpte].is1g()) {
        return PageSize2::eHuge;
    }

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return PageSize2::eNone;

    if (l2->entries[pdte].is2m()) {
        return PageSize2::eLarge;
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return PageSize2::eNone;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return PageSize2::eNone;

    return PageSize2::eRegular;
}

km::PhysicalAddress PageTables::asPhysical(const void *ptr) const {
    return km::PhysicalAddress { (uintptr_t)ptr - mSlide };
}

OsStatus PageTables::map(MappingRequest request, AddressMapping *mapping) {

}

OsStatus PageTables::unmap(AddressMapping mapping) {

}
