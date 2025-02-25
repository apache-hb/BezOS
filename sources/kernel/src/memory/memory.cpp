#include "memory/memory.hpp"
#include "log.hpp"

using PageTables = km::PageTables;

km::PageWalkIndices km::GetAddressParts(const void *ptr) {
    return GetAddressParts(reinterpret_cast<uintptr_t>(ptr));
}

km::AddressMapping km::detail::AlignLargeRangeHead(AddressMapping mapping) {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t aligned = sm::roundup(vaddr, x64::kLargePageSize);

    return AddressMapping {
        .vaddr = mapping.vaddr,
        .paddr = mapping.paddr,
        .size = aligned - vaddr,
    };
}

km::AddressMapping km::detail::AlignLargeRangeBody(AddressMapping mapping) {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    PhysicalAddress paddr = mapping.paddr;

    uintptr_t head2m = sm::roundup(vaddr, x64::kLargePageSize);
    uintptr_t tail2m = sm::rounddown(vaddr + mapping.size, x64::kLargePageSize);
    size_t offset = head2m - vaddr;
    size_t size = tail2m - head2m;

    return AddressMapping {
        .vaddr = reinterpret_cast<void*>(head2m),
        .paddr = paddr + offset,
        .size = size
    };
}

km::AddressMapping km::detail::AlignLargeRangeTail(AddressMapping mapping) {
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);
    uintptr_t tail2m = sm::rounddown(vaddr + mapping.size, x64::kLargePageSize);
    size_t size = vaddr + mapping.size - tail2m;

    return AddressMapping {
        .vaddr = reinterpret_cast<void*>(tail2m),
        .paddr = mapping.paddr + mapping.size - size,
        .size = size
    };
}

bool km::detail::IsLargePageEligible(AddressMapping mapping) {
    //
    // Ranges smaller than 2m are not eligible for large pages.
    //
    if (mapping.size < x64::kLargePageSize) return false;

    uintptr_t paddr = mapping.paddr.address;
    uintptr_t vaddr = reinterpret_cast<uintptr_t>(mapping.vaddr);

    //
    // If the addresses are not aligned equally, the range is not eligible for large pages.
    // e.g. if the physical address is at 0x202000 and the virtual address is at 0x202000, the range is eligible.
    // But if the physical address is at 0x202000 and the virtual address is at 0x203000, the range is not eligible.
    //
    uintptr_t mask = x64::kLargePageSize - 1;
    if ((paddr & mask) != (vaddr & mask)) return false;

    //
    // After aligning the front of the range up to a 2m boundary and the size of the range down
    // to a 2m boundary, the range must still be larger than 2m to be eligible for large pages.
    //
    uintptr_t front2m = sm::roundup(paddr, x64::kLargePageSize);
    uintptr_t back2m = sm::rounddown(paddr + mapping.size, x64::kLargePageSize);

    return front2m < back2m;
}

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

    return nullptr;
}

void PageTables::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
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
    const x64::pdte& pde = l2->entries[pdte];
    if (pde.present() && !pde.is2m()) {
        uintptr_t base = mPageManager->address(pde);
        return asVirtual<x64::PageTable>(base);
    }

    return nullptr;
}

OsStatus PageTables::mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kPageSize) {
        if (OsStatus status = map4k(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus PageTables::mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kLargePageSize) {
        if (OsStatus status = map2m(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus PageTables::mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kHugePageSize) {
        if (OsStatus status = map1g(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
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

    auto [pml4e, pdpte, pdte, pte] = GetAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eUserAll);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, PageFlags::eUserAll);
    if (!l2) return OsStatusOutOfMemory;

    x64::PageTable *pt;

    x64::pdte& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(allocPage());
        if (!pt) return OsStatusOutOfMemory;

        setEntryFlags(t2, PageFlags::eUserAll, asPhysical(pt));
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

    auto [pml4e, pdpte, pdte, _] = GetAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eUserAll);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, PageFlags::eUserAll);
    if (!l2) return OsStatusOutOfMemory;

    x64::pdte& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr);

    return OsStatusSuccess;
}

OsStatus PageTables::map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    stdx::LockGuard guard(mLock);

    auto [pml4e, pdpte, _, _] = GetAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, PageFlags::eUserAll);
    if (!l3) return OsStatusOutOfMemory;

    x64::pdpte& t3 = l3->entries[pdpte];
    t3.set1g(true);
    mPageManager->setMemoryType(t3, type);
    setEntryFlags(t3, flags, paddr);

    return OsStatusSuccess;
}

void PageTables::unmap(void *ptr, size_t size) {
    uintptr_t front = sm::rounddown((uintptr_t)ptr, x64::kPageSize);
    uintptr_t back = sm::roundup((uintptr_t)ptr + size, x64::kPageSize);

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel4 *l4 = getRootTable();

    for (uintptr_t i = front; i < back; i += x64::kPageSize) {
        auto [pml4e, pdpte, pdte, pte] = GetAddressParts(i);

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

km::PhysicalAddress PageTables::getBackingAddress(const void *ptr) {
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    auto [pml4e, pdpte, pdte, pte] = GetAddressParts(ptr);
    uintptr_t offset = address & 0xFFF;

    stdx::LockGuard guard(mLock);

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

km::PageFlags PageTables::getMemoryFlags(const void *ptr) {
    PageWalk result;
    if (walk(ptr, &result) != OsStatusSuccess) {
        return PageFlags::eNone;
    }

    return result.flags();
}

km::PageSize2 PageTables::getPageSize(const void *ptr) {
    PageWalk result;
    if (walk(ptr, &result) != OsStatusSuccess) {
        return PageSize2::eNone;
    }

    return result.pageSize();
}

km::PhysicalAddress PageTables::asPhysical(const void *ptr) const {
    return km::PhysicalAddress { (uintptr_t)ptr - mSlide };
}

OsStatus PageTables::map(MappingRequest request) {
    AddressMapping mapping = request.mapping;
    PageFlags flags = request.flags;
    MemoryType type = request.type;

    KM_CHECK(mapping.size % x64::kPageSize == 0, "Range size must be page aligned.");
    KM_CHECK((uintptr_t)mapping.vaddr % x64::kPageSize == 0, "Virtual address must be page aligned.");
    KM_CHECK(mapping.paddr.address % x64::kPageSize == 0, "Physical address must be page aligned.");

    //
    // We can use large pages if the range is larger than 2m after alignment and the mapping
    // has equal alignment between the physical and virtual addresses relative to the 2m boundary.
    //
    if (detail::IsLargePageEligible(mapping)) {
        //
        // If the range is larger than 2m in total, check if
        // the range is still larger than 2m after aligning the range.
        //
        MemoryRange range = mapping.physicalRange();
        PhysicalAddress front2m = sm::roundup(range.front.address, x64::kLargePageSize);
        PhysicalAddress back2m = sm::rounddown(range.back.address, x64::kLargePageSize);

        //
        // If the range is still larger than 2m, we can use 2m pages.
        //
        if (front2m < back2m) {
            //
            // Map the leading 4k pages we need to map to fulfill our api contract.
            //
            AddressMapping head = detail::AlignLargeRangeHead(mapping);
            if (!head.isEmpty()) {
                if (OsStatus status = mapRange4k(head, flags, type)) {
                    return status;
                }
            }

            //
            // Then map the middle 2m pages.
            //
            AddressMapping body = detail::AlignLargeRangeBody(mapping);
            if (OsStatus status = mapRange2m(body, flags, type)) {
                return status;
            }

            //
            // Finally map the trailing 4k pages.
            //
            AddressMapping tail = detail::AlignLargeRangeTail(mapping);
            if (!tail.isEmpty()) {
                if (OsStatus status = mapRange4k(tail, flags, type)) {
                    return status;
                }
            }

            return OsStatusSuccess;
        }
    }

    //
    // If we get to this point its not worth using 2m pages
    // so we map the range with 4k pages.
    //
    return mapRange4k(mapping, flags, type);
}

OsStatus PageTables::unmap(AddressMapping mapping) {
    unmap((void*)mapping.vaddr, mapping.size);

    return OsStatusSuccess;
}

OsStatus PageTables::walk(const void *ptr, PageWalk *walk) {
    auto [pml4eIndex, pdpteIndex, pdteIndex, pteIndex] = GetAddressParts(ptr);
    x64::pml4e pml4e{};
    x64::pdpte pdpte{};
    x64::pdte pdte{};
    x64::pte pte{};

    stdx::LockGuard guard(mLock);

    //
    // Small trick with do { } while loops to achive goto-like behaviour while avoiding
    // undefined behaviour from jumping over variable initialization.
    // If any of these steps fail we want to return the data we have so far, so
    // we break out of the loop and return the data we've collected.
    //
    do {
        const x64::PageMapLevel4 *l4 = getRootTable();
        pml4e = l4->entries[pml4eIndex];
        if (!pml4e.present()) break;

        const x64::PageMapLevel3 *l3 = getPageEntry<x64::PageMapLevel3>(l4, pml4eIndex);
        pdpte = l3->entries[pdpteIndex];
        if (!pdpte.present() || pdpte.is1g()) break;

        const x64::PageMapLevel2 *l2 = getPageEntry<x64::PageMapLevel2>(l3, pdpteIndex);
        pdte = l2->entries[pdteIndex];
        if (!pdte.present() || pdte.is2m()) break;

        x64::PageTable *pt = getPageEntry<x64::PageTable>(l2, pdteIndex);
        pte = pt->entries[pteIndex];
    } while (false);

    PageWalk result {
        .address = reinterpret_cast<uintptr_t>(ptr),
        .pml4eIndex = pml4eIndex,
        .pml4e = pml4e,
        .pdpteIndex = pdpteIndex,
        .pdpte = pdpte,
        .pdteIndex = pdteIndex,
        .pdte = pdte,
        .pteIndex = pteIndex,
        .pte = pte,
    };

    *walk = result;
    return OsStatusSuccess;
}
