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

    mPageManager->setAddress(entry, address);

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
    KM_CHECK(pde.is2m(), "Splitting pde that is already split.");

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

void PageTables::cut2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase, x64::PageTable *pt) {
    VirtualRange remaining = page.cut(erase);

    MemoryType type = mPageManager->getMemoryType(pde);
    PhysicalAddress paddr = mPageManager->address(pde);
    uintptr_t offset = (uintptr_t)remaining.front - (uintptr_t)page.front;

    //
    // Remap the area that is still valid.
    //
    partialRemap2m(pt, remaining, paddr + offset, type);

    //
    // Update the 2m page to point to the new 4k page table.
    //
    setEntryFlags(pde, mMiddleFlags, asPhysical(pt));
    pde.set2m(false);
}

void PageTables::reclaim2m(x64::pdte& pde) {
    //
    // When unmapping a 2m page we need to determine if we can reclaim a page table.
    //
    if (!pde.is2m()) {
        //
        // If the page is a set of 4k pages we can reclaim the page table.
        //
        x64::PageTable *pt = asVirtual<x64::PageTable>(mPageManager->address(pde));
        mAllocator.deallocate(pt, sizeof(x64::PageTable));
    }

    pde.setPresent(false);
    pde.set2m(false);
}

OsStatus PageTables::unmap2mRegion(x64::pdte& pde, uintptr_t address, VirtualRange range) {
    VirtualRange page = VirtualRange::of((void*)sm::rounddown(address, x64::kLargePageSize), x64::kLargePageSize);

    //
    // If the 2m page is entirely contained within the unmap request we can remove it
    // without any extra work.
    //
    if (range.contains(page)) {
        reclaim2m(pde);
        x64::invlpg(address);

        return OsStatusSuccess;
    }

    //
    // If we reach this point we know that the unmap request is entirely within a 2m page.
    //
    KM_CHECK(page.contains(range) && page.front != range.front && page.back != range.back, "Invalid unmap range.");
    return split2mMapping(pde, page, range);
}

int PageTables::earlyAllocatePageTables(VirtualRange range) {
    auto isMappedUsingLargePage = [&](const void *address) {
        PageWalk walk = walkUnlocked(address);
        return walk.pageSize() == PageSize::eLarge;
    };

    //
    // If either the front or back of the range is aligned to a 2m page boundary then we
    // dont need to pre-allocate any page tables.
    //
    bool frontAligned = (uintptr_t)range.front % x64::kLargePageSize == 0;
    bool backAligned = (uintptr_t)range.back % x64::kLargePageSize == 0;
    if (frontAligned && backAligned) {
        return 0;
    }

    //
    // If both ends of the range are in the same 2m page then only 1 page table is required.
    //
    if (sm::rounddown((uintptr_t)range.front, x64::kLargePageSize) == sm::rounddown((uintptr_t)range.back, x64::kLargePageSize)) {
        return isMappedUsingLargePage(range.front) ? 1 : 0;
    }

    //
    // Check if the head and tail ranges are mapped. We know both these addresses
    // are not 2m aligned so we need to check if the mappings are 2m pages.
    //
    int count = 0;
    if (!frontAligned && isMappedUsingLargePage(range.front)) {
        count += 1;
    }

    if (!backAligned && isMappedUsingLargePage(range.back)) {
        count += 1;
    }

    return count;
}

x64::pdte& PageTables::getLargePageEntry(const void *address) {
    auto [pml4e, pdpte, pdte, _] = GetAddressParts(address);

    x64::PageMapLevel4 *l4 = pml4();

    x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    KM_CHECK(l3 != nullptr, "Failed to find page map level 3.");

    x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    KM_CHECK(l2 != nullptr, "Failed to find page map level 2.");

    return l2->entries[pdte];
}

PageWalk PageTables::walkUnlocked(const void *ptr) const {
    auto [pml4eIndex, pdpteIndex, pdteIndex, pteIndex] = GetAddressParts(ptr);
    x64::pml4e pml4e{};
    x64::pdpte pdpte{};
    x64::pdte pdte{};
    x64::pte pte{};

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

    return PageWalk {
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
}

OsStatus PageTables::earlyUnmap(VirtualRange range, VirtualRange *remaining) {
    //
    // Fun edge case when unampping a range of memory that isnt 2m aligned but intersects with 2 2m pages.
    // As this requires allocating 2 new page tables to store the 4k mappings required: the first allocation
    // can succeed and the second can fail. If we did the allocating while iterating we could reach a state
    // where the second allocation fails but page tables have already been manipulated. So we need to allocate both
    // required page tables upfront to ensure both exist before unmapping the remaining area.
    //
    int earlyAllocations = earlyAllocatePageTables(range);
    if (earlyAllocations == 2) {
        x64::PageTable *ptlow = std::bit_cast<x64::PageTable*>(alloc4k());
        if (ptlow == nullptr) {
            return OsStatusOutOfMemory;
        }

        x64::PageTable *pthigh = std::bit_cast<x64::PageTable*>(alloc4k());
        if (pthigh == nullptr) {
            mAllocator.deallocate(ptlow, sizeof(x64::PageTable));
            return OsStatusOutOfMemory;
        }

        //
        // We rebuild the first and last mappings early here, then shrink the range and continue the
        // unmapping iteration.
        //
        VirtualRange low { range.front, (void*)sm::roundup((uintptr_t)range.front, x64::kLargePageSize) };
        VirtualRange high { (void*)sm::rounddown((uintptr_t)range.back, x64::kLargePageSize), range.back };

        VirtualRange lowPage {
            (void*)sm::rounddown((uintptr_t)range.front, x64::kLargePageSize),
            (void*)sm::roundup((uintptr_t)range.front, x64::kLargePageSize)
        };

        VirtualRange highPage {
            (void*)sm::rounddown((uintptr_t)range.back, x64::kLargePageSize),
            (void*)sm::roundup((uintptr_t)range.back, x64::kLargePageSize)
        };

        x64::pdte& lowEntry = getLargePageEntry(low.front);
        x64::pdte& highEntry = getLargePageEntry(high.front);

        cut2mMapping(lowEntry, lowPage, low, ptlow);
        cut2mMapping(highEntry, highPage, high, pthigh);

        *remaining = VirtualRange { low.back, high.front };
    } else if (earlyAllocations == 1) {
        VirtualRange page {
            (void*)sm::rounddown((uintptr_t)range.front, x64::kLargePageSize),
            (void*)sm::roundup((uintptr_t)range.back, x64::kLargePageSize)
        };

        //
        // If both ends of the range are inside the same 2m page then we need to use split2mMapping
        // rather than cut2mMapping.
        //
        if (page.contains(range) && !innerAdjacent(page, range)) {
            x64::pdte& entry = getLargePageEntry(range.front);
            if (OsStatus status = split2mMapping(entry, page, range)) {
                return status;
            }

            *remaining = VirtualRange{};
            return OsStatusSuccess;
        }

        //
        // If we get here we know that the range is not entirely contained within a 2m page.
        // So we need to allocate early in case the unmap ends partially in the last 2m page.
        //
        x64::PageTable *pt = std::bit_cast<x64::PageTable*>(alloc4k());
        if (pt == nullptr) {
            return OsStatusOutOfMemory;
        }

        if (page.front != range.front) {
            x64::pdte& entry = getLargePageEntry(range.front);
            cut2mMapping(entry, page, range, pt);
        } else {
            KM_CHECK(page.back != range.back, "Invalid range.");
            x64::pdte& entry = getLargePageEntry(range.front);
            cut2mMapping(entry, page, range, pt);
        }

        *remaining = VirtualRange{};
    }

    return OsStatusSuccess;
}

OsStatus PageTables::unmap(VirtualRange range) {
    range = alignedOut(range, x64::kPageSize);

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel4 *l4 = pml4();

    if (OsStatus status = earlyUnmap(range, &range)) {
        return status;
    }

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

OsStatus PageTables::unmap2m(VirtualRange range) {
    //
    // Test preconditions to ensure the range is valid for 2m unmap.
    //
    bool valid = ((uintptr_t)range.front % x64::kLargePageSize == 0)
        && ((uintptr_t)range.back % x64::kLargePageSize == 0)
        && range.size() >= x64::kLargePageSize;

    if (!valid) {
        return OsStatusInvalidInput;
    }

    stdx::LockGuard guard(mLock);

    x64::PageMapLevel4 *l4 = pml4();

    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back;) {
        auto [pml4e, pdpte, pdte, _] = GetAddressParts(i);

        //
        // If we can't find the page table for this range, then skip over the inaccessable address space.
        //
        x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
        if (!l3) {
            i += x64::kHugePageSize * 512;
            continue;
        }

        x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
        if (!l2) {
            i += x64::kHugePageSize;
            continue;
        }

        x64::pdte& pde = l2->entries[pdte];
        if (!pde.present()) {
            i += x64::kLargePageSize;
            continue;
        }

        //
        // When unmapping a 2m page we need to determine if we can reclaim a page table.
        //
        reclaim2m(pde);

        x64::invlpg(i);
        i += x64::kLargePageSize;
    }

    return OsStatusSuccess;
}

PhysicalAddress PageTables::getBackingAddressUnlocked(const void *ptr) const {
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    auto [pml4e, pdpte, pdte, pte] = GetAddressParts(ptr);
    uintptr_t offset = address & 0xFFF;

    const x64::PageMapLevel4 *l4 = pml4();
    const x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) return KM_INVALID_MEMORY;

    const x64::PageMapLevel3 *l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) return KM_INVALID_MEMORY;

    if (t3.is1g()) {
        uintptr_t pdpteOffset = address & 0x3FF'000;
        return km::PhysicalAddress { mPageManager->address(t3) + pdpteOffset + offset };
    }

    const x64::PageMapLevel2 *l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    const x64::pdte& t2 = l2->entries[pdte];
    if (!t2.present()) return KM_INVALID_MEMORY;

    if (t2.is2m()) {
        uintptr_t pdteOffset = address & 0x1FF'000;
        return km::PhysicalAddress { mPageManager->address(t2) + pdteOffset + offset };
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return KM_INVALID_MEMORY;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return KM_INVALID_MEMORY;

    return km::PhysicalAddress { mPageManager->address(t1) + offset };
}

km::PhysicalAddress PageTables::getBackingAddress(const void *ptr) {
    stdx::LockGuard guard(mLock);
    return getBackingAddressUnlocked(ptr);
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
    stdx::LockGuard guard(mLock);

    *walk = walkUnlocked(ptr);
    return OsStatusSuccess;
}
