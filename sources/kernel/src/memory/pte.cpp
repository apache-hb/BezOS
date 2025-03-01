#include "memory/pte.hpp"

#include "log.hpp"

using namespace km;

x64::page *PageTables::alloc4k() {
    if (x64::page *page = mAllocator.construct<x64::page>()) {
        memset(page, 0, sizeof(x64::page));

        return page;
    }

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

PageTables::PageTables(const km::PageBuilder *pm, AddressMapping pteMemory, PageFlags middleFlags)
    : mSlide(pteMemory.slide())
    , mAllocator((void*)pteMemory.vaddr, pteMemory.size)
    , mPageManager(pm)
    , mRootPageTable((x64::PageMapLevel4*)alloc4k())
    , mMiddleFlags(middleFlags)
{ }

x64::PageMapLevel3 *PageTables::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
        if (l3) setEntryFlags(t4, mMiddleFlags, asPhysical(l3));
    } else {
        l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *PageTables::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
        if (l2) setEntryFlags(t3, mMiddleFlags, asPhysical(l2));
    } else {
        l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    }

    return l2;
}

x64::PageMapLevel3 *PageTables::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return asVirtual<x64::PageMapLevel3>(base);
    }

    return nullptr;
}

x64::PageMapLevel2 *PageTables::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const {
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

    if (!mPageManager->isCanonicalAddress(vaddr)) {
        KmDebugMessage("Attempting to map address that isn't canonical: ", vaddr, "\n");
        KM_PANIC("Invalid memory mapping.");
    }

    auto [pml4e, pdpte, pdte, pte] = GetAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);
    if (!l2) return OsStatusOutOfMemory;

    x64::PageTable *pt;

    x64::pdte& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(alloc4k());
        if (!pt) return OsStatusOutOfMemory;

        setEntryFlags(t2, mMiddleFlags, asPhysical(pt));
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

    if (!mPageManager->isCanonicalAddress(vaddr)) {
        KmDebugMessage("Attempting to map address that isn't canonical: ", vaddr, "\n");
        KM_PANIC("Invalid memory mapping.");
    }

    auto [pml4e, pdpte, pdte, _] = GetAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    if (!l3) return OsStatusOutOfMemory;

    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);
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

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    if (!l3) return OsStatusOutOfMemory;

    x64::pdpte& t3 = l3->entries[pdpte];
    t3.set1g(true);
    mPageManager->setMemoryType(t3, type);
    setEntryFlags(t3, flags, paddr);

    return OsStatusSuccess;
}

OsStatus PageTables::map(AddressMapping mapping, PageFlags flags, MemoryType type) {
    bool valid = (mapping.size % x64::kPageSize == 0)
        && (mapping.paddr.address % x64::kPageSize == 0)
        && ((uintptr_t)mapping.vaddr % x64::kPageSize == 0)
        && mPageManager->isCanonicalAddress(mapping.vaddr);

    if (!valid) {
        KmDebugMessage("[MEM] Invalid mapping request ", mapping, "\n");
        return OsStatusInvalidInput;
    }

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

OsStatus PageTables::map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    return map(MappingOf(range, vaddr), flags, type);
}

void PageTables::partialRemap2m(x64::PageTable *pt, VirtualRange range, PhysicalAddress paddr, MemoryType type) {
    KM_CHECK(!range.isEmpty(), "Cannot remap empty range.");

    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back; i += x64::kPageSize) {
        auto [pml4e, pdpte, pdte, pte] = GetAddressParts(i);
        x64::pte& t1 = pt->entries[pte];
        mPageManager->setMemoryType(t1, type);
        setEntryFlags(t1, mMiddleFlags, paddr + (i - (uintptr_t)range.front));
    }
}

OsStatus PageTables::split2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase) {
    auto [lo, hi] = km::split(page, erase);
    MemoryType type = mPageManager->getMemoryType(pde);
    PhysicalAddress paddr = mPageManager->address(pde);
    uintptr_t hiOffset = (uintptr_t)erase.back - (uintptr_t)page.front;

    x64::PageTable *pt = std::bit_cast<x64::PageTable*>(alloc4k());
    if (!pt) return OsStatusOutOfMemory;

    //
    // Map the lo part of the remaining area.
    //
    partialRemap2m(pt, lo, paddr, type);

    //
    // The map the hi part of the area.
    //
    partialRemap2m(pt, hi, paddr + hiOffset, type);

    //
    // Finally, update the 2m page to point to the new 4k page table.
    //
    setEntryFlags(pde, mMiddleFlags, asPhysical(pt));
    pde.set2m(false);

    return OsStatusSuccess;
}

OsStatus PageTables::cut2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase) {
    VirtualRange remaining = page.cut(erase);

    MemoryType type = mPageManager->getMemoryType(pde);
    PhysicalAddress paddr = mPageManager->address(pde);
    uintptr_t offset = (uintptr_t)remaining.front - (uintptr_t)page.front;

    x64::PageTable *pt = std::bit_cast<x64::PageTable*>(alloc4k());
    if (!pt) return OsStatusOutOfMemory;

    //
    // Remap the area that is still valid.
    //
    partialRemap2m(pt, remaining, paddr + offset, type);

    //
    // Update the 2m page to point to the new 4k page table.
    //
    setEntryFlags(pde, mMiddleFlags, asPhysical(pt));
    pde.set2m(false);

    return OsStatusSuccess;
}

OsStatus PageTables::unmap2mRegion(x64::pdte& pde, uintptr_t address, VirtualRange range) {
    VirtualRange page = VirtualRange::of((void*)sm::rounddown(address, x64::kLargePageSize), x64::kLargePageSize);

    //
    // If the 2m page is entirely contained within the unmap request we can remove it
    // without any extra work.
    //
    if (range.contains(page)) {
        pde.setPresent(false);
        pde.set2m(false);
        x64::invlpg(address);

        return OsStatusSuccess;
    }

    //
    // If the unmap range is a subset of the page range then we need to replace
    // the 2m page with 4k pages that map the equivalent area. If the unmap range
    // borders the 2m page then we need to cut rather than split.
    //
    if (page.contains(range) && page.front != range.front && page.back != range.back) {
        return split2mMapping(pde, page, range);
    } else {
        //
        // If the page isnt fully contained within the unmap range then we need to remove
        // either the front or back of the range and replace it with 4k pages.
        //
        return cut2mMapping(pde, page, range);
    }
}

OsStatus PageTables::unmap(VirtualRange range) {
    range = alignedOut(range, x64::kPageSize);

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel4 *l4 = pml4();

    uintptr_t i = (uintptr_t)range.front;
    uintptr_t end = (uintptr_t)range.back;

    //
    // Iterate over the range we want to unmap and unmap the pages.
    //
    while (i < end) {
        auto [pml4e, pdpte, pdte, pte] = GetAddressParts(i);

        //
        // If we can't find the page table for this range, then skip over the inaccessable address space.
        //
        x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
        if (!l3) {
            i += sm::nextMultiple(i, x64::kHugePageSize * 512);
            continue;
        }

        x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
        if (!l2) {
            i += sm::nextMultiple(i, x64::kHugePageSize);
            continue;
        }

        x64::pdte& pde = l2->entries[pdte];
        if (!pde.present()) {
            i += sm::nextMultiple(i, x64::kLargePageSize);
            continue;
        }

        //
        // If we are unmapping a 2m page, we need to determine if we can unmap the entire page
        // or if we need to remap part of the 2m page as 4k pages.
        //
        if (pde.is2m()) {
            unmap2mRegion(pde, i, range);
            i = sm::nextMultiple(i, x64::kLargePageSize);
            continue;
        }

        x64::PageTable *pt = findPageTable(l2, pdte);
        if (!pt) {
            i += x64::kPageSize;
            continue;
        }

        x64::pte& t1 = pt->entries[pte];
        t1.setPresent(false);
        x64::invlpg(i);

        i += x64::kPageSize;
    }

    return OsStatusSuccess;
}

km::PhysicalAddress PageTables::getBackingAddress(const void *ptr) {
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    auto [pml4e, pdpte, pdte, pte] = GetAddressParts(ptr);
    uintptr_t offset = address & 0xFFF;

    stdx::LockGuard guard(mLock);

    const x64::PageMapLevel4 *l4 = pml4();
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

km::PageSize PageTables::getPageSize(const void *ptr) {
    PageWalk result;
    if (walk(ptr, &result) != OsStatusSuccess) {
        return PageSize::eNone;
    }

    return result.pageSize();
}

km::PhysicalAddress PageTables::asPhysical(const void *ptr) const {
    return km::PhysicalAddress { (uintptr_t)ptr - mSlide };
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
        const x64::PageMapLevel4 *l4 = pml4();
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
        .address = ptr,
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
