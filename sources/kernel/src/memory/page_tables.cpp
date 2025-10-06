#include "memory/page_tables.hpp"

#include "common/util/defer.hpp"
#include "logger/categories.hpp"

using namespace km;

x64::page *PageTables::alloc4k() [[clang::allocating]] {
    x64::page *it = (x64::page*)mAllocator.allocate(1).getVirtual();

    if (it) {
        memset(it, 0, sizeof(x64::page));
    }

    return it;
}

void PageTables::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddressEx address) noexcept [[clang::nonblocking]] {
    if (address.address > mPageManager->maxPhysicalAddress()) {
        KM_PANIC("Physical address out of range.");
    }

    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));
    entry.setUser(bool(flags & PageFlags::eUser));

    entry.setPresent(true);
}

OsStatus PageTables::trackMapping(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    return mCache.addMemory(mapping);
}

void PageTables::deallocate(sm::VirtualAddress vaddr, sm::PhysicalAddress paddr, size_t pages) noexcept [[clang::nonallocating]] {
    uintptr_t slide = paddr.address - vaddr.address;
    mAllocator.deallocate(vaddr, pages, slide);
}

OsStatus PageTables::create(const PageBuilder *pm, AddressMapping pteMemory, PageFlags flags, PageTables *tables [[outparam]]) [[clang::allocating]] {
    if (OsStatus status = PageTableAllocator::create(pteMemory, x64::kPageSize, &tables->mAllocator)) {
        return status;
    }

    PageTableAllocation root = tables->mAllocator.allocate(1);
    PageTableAllocation cache = tables->mAllocator.allocate(1);

    if (!root.isPresent() || !cache.isPresent()) {
        return OsStatusOutOfMemory;
    }

    memset(root.getVirtual(), 0, x64::kPageSize);

    tables->mPageManager = pm;
    tables->mMiddleFlags = flags;
    tables->mRootAllocation = root;
    tables->mCache = detail::MappingLookupTable{cache.getVirtual(), x64::kPageSize};

    OsStatus status = tables->trackMapping(pteMemory);
    KM_CHECK(status == OsStatusSuccess, "Failed to track page table memory");

    return OsStatusSuccess;
}

x64::PageMapLevel3 *PageTables::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        PageTableAllocation allocation = buffer.next();
        l3 = std::bit_cast<x64::PageMapLevel3*>(allocation.getVirtual());
        setEntryFlags(t4, mMiddleFlags, asPhysical(allocation));
    } else {
        l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *PageTables::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        PageTableAllocation allocation = buffer.next();
        l2 = std::bit_cast<x64::PageMapLevel2*>(allocation.getVirtual());
        setEntryFlags(t3, mMiddleFlags, asPhysical(allocation));
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

void PageTables::mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mapping.size; i += x64::kPageSize) {
        map4k(mapping.paddr.address + i, (char*)mapping.vaddr + i, flags, type, buffer);
    }
}

void PageTables::mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mapping.size; i += x64::kLargePageSize) {
        map2m(mapping.paddr.address + i, (char*)mapping.vaddr + i, flags, type, buffer);
    }
}

void PageTables::mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) {
    for (size_t i = 0; i < mapping.size; i += x64::kHugePageSize) {
        map1g(mapping.paddr.address + i, (char*)mapping.vaddr + i, flags, type, buffer);
    }
}

void PageTables::map4k(PhysicalAddressEx paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    auto [pml4e, pdpte, pdte, pte] = getAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, buffer);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, buffer);
    x64::PageTable *pt;

    x64::pdte& t2 = l2->entries[pdte];
    if (!t2.present()) {
        PageTableAllocation allocation = buffer.next();
        pt = std::bit_cast<x64::PageTable*>(allocation.getVirtual());
        setEntryFlags(t2, mMiddleFlags, asPhysical(allocation));
    } else if (t2.is2m()) {
        //
        // We need to split the 2m mapping to make space for this mapping.
        //
        PageTableAllocation allocation = buffer.next();
        pt = std::bit_cast<x64::PageTable*>(allocation.getVirtual());
        uintptr_t front2m = sm::rounddown((uintptr_t)vaddr, x64::kLargePageSize);
        uintptr_t back2m = sm::roundup((uintptr_t)vaddr, x64::kLargePageSize);
        VirtualRange page = { (void*)front2m, (void*)back2m };
        VirtualRange erase = { (void*)vaddr, (void*)((uintptr_t)vaddr + x64::kPageSize) };
        split2mMapping(t2, page, erase, allocation);
    } else {
        pt = asVirtual<x64::PageTable>(mPageManager->address(t2));
    }

    x64::pte& t1 = pt->entries[pte];
    mPageManager->setMemoryType(t1, type);
    setEntryFlags(t1, flags, paddr);
}

void PageTables::map2m(PhysicalAddressEx paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    auto [pml4e, pdpte, pdte, _] = getAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, buffer);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte, buffer);
    x64::pdte& t2 = l2->entries[pdte];

    //
    // If this is a pdte entry that is already present but not a 2m mapping we need to free
    // the old page table so we dont leak when we replace this entry with a 2m mapping.
    //
    if (t2.present() && !t2.is2m()) {
        sm::PhysicalAddress paddr = mPageManager->address(t2);
        void *ptr = asVirtual<x64::PageTable>(paddr);
        deallocate(ptr, paddr, 1);
    }

    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr);
}

void PageTables::map1g(PhysicalAddressEx paddr, const void *vaddr, PageFlags flags, MemoryType type, detail::PageTableList& buffer) {
    auto [pml4e, pdpte, _, _] = getAddressParts(vaddr);

    x64::PageMapLevel4 *l4 = pml4();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e, buffer);

    x64::pdpte& t3 = l3->entries[pdpte];
    t3.set1g(true);
    mPageManager->setMemoryType(t3, type);
    setEntryFlags(t3, flags, paddr);
}

size_t PageTables::maxPagesForMapping(VirtualRange range) const {
    //
    // Pre allocate enough tables to map the entire range, in cases of oom we want to not map anything
    // rather than partially map the range. TODO: use a more accurate calculation rather than overallocating
    // and releasing the unused tables.
    //
    return detail::MaxPagesForMapping(range);
}

size_t PageTables::countPagesForMapping(VirtualRange range) {
    size_t count = 0;

    //
    // Walks page tables to figure out how many tables would need to be allocated
    // to map a range. This is used if allocating the worst case number of tables fails.
    // TODO: this could be more efficient with better table walking logic.
    //

    const x64::PageMapLevel4 *l4 = pml4();

    {
        //
        // First count the number of pml4 entries we need to allocate.
        //
        constexpr static auto kTopPageSize = x64::kHugePageSize * 512;
        VirtualRange aligned = km::alignedOut(range, kTopPageSize);
        for (uintptr_t i = (uintptr_t)aligned.front; i < (uintptr_t)aligned.back; i += kTopPageSize) {
            auto [pml4e, _, _, _] = getAddressParts((void*)i);
            const x64::pml4e& t4 = l4->entries[pml4e];
            if (!t4.present()) {
                count += 1;
            }
        }
    }

    {
        //
        // Then count the number of pdpt entries we need to allocate.
        //
        VirtualRange aligned = km::alignedOut(range, x64::kHugePageSize);
        for (uintptr_t i = (uintptr_t)aligned.front; i < (uintptr_t)aligned.back; i += x64::kHugePageSize) {
            auto [pml4e, pdpte, _, _] = getAddressParts((void*)i);
            const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
            if (l3 == nullptr) {
                count += 1;
                continue;
            }

            const x64::pdpte& t3 = l3->entries[pdpte];
            if (!t3.present()) {
                count += 1;
            }
        }
    }

    {
        //
        // Then count the number of pd entries we need to allocate.
        //
        VirtualRange aligned = km::alignedOut(range, x64::kLargePageSize);
        for (uintptr_t i = (uintptr_t)aligned.front; i < (uintptr_t)aligned.back; i += x64::kLargePageSize) {
            auto [pml4e, pdpte, pdte, _] = getAddressParts((void*)i);
            const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
            if (l3 == nullptr) {
                count += 1;
                continue;
            }

            const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
            if (l2 == nullptr) {
                count += 1;
                continue;
            }

            const x64::pdte& t2 = l2->entries[pdte];
            if (!t2.present()) {
                count += 1;
            }
        }
    }

    //
    // If the front or back of the range is not aligned to a 2m boundary we need to allocate
    // extra tables to map the ends.
    //
    if ((uintptr_t)range.front % x64::kLargePageSize != 0) {
        count += 1;
    }

    if ((uintptr_t)range.back % x64::kLargePageSize != 0) {
        count += 1;
    }

    return count;
}

OsStatus PageTables::allocatePageTables(VirtualRange range, detail::PageTableList *list) {
    //
    // Pre-allocate enough page tables to fullfill the request.
    //
    size_t requiredPages = maxPagesForMapping(range);
    if (mAllocator.allocateList(requiredPages, list)) {
        return OsStatusSuccess;
    }

    //
    // If the fast path fails then do a more expensive page table scan to
    // figure out the exact number of tables we need to allocate.
    //
    MemLog.warnf("Low memory, failed to allocate ", requiredPages, " page tables. Retrying with conservative page count.");

    auto stats = mAllocator.stats();
    MemLog.warnf("   Free pages: ", stats.freeBlocks);
    MemLog.warnf("Largest block: ", stats.largestBlock);

    requiredPages = countPagesForMapping(range);
    if (mAllocator.allocateList(requiredPages, list)) {
        return OsStatusSuccess;
    }

    //
    // Last ditch effort to try and fulfill the request, compaction can be very expensive.
    //
    MemLog.warnf("Critically low memory. Performing emergency page compaction.");
    size_t count = compactUnlocked();
    MemLog.warnf("Compacted ", count, " pages.");

    if (mAllocator.allocateList(requiredPages, list)) {
        return OsStatusSuccess;
    }

    //
    // If both allocations fail then we are out of memory.
    //
    return OsStatusOutOfMemory;
}

OsStatus PageTables::reservePageTablesForMapping(VirtualRange range, detail::PageTableList& list) {
    auto allocateExtra = [&](size_t count) {
        if (count == 0) {
            return true;
        }

        if (mAllocator.allocateExtra(count, list)) {
            return true;
        }

        return false;
    };

    if (allocateExtra(maxPagesForMapping(range))) {
        return OsStatusSuccess;
    }

    if (allocateExtra(countPagesForMapping(range))) {
        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}

OsStatus PageTables::reservePageTablesForUnmapping(VirtualRange range, detail::PageTableList& list) {
    if (int pteCount = countRequiredPageTables(range)) {
        if (OsStatus status = mAllocator.allocateExtra(pteCount, list)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

void PageTables::drainTableList(detail::PageTableList list) noexcept [[clang::nonallocating]] {
    mAllocator.deallocateList(std::move(list));
}

void PageTables::mapWithList(AddressMapping mapping, PageFlags flags, MemoryType type, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
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
                mapRange4k(head, flags, type, buffer);
            }

            //
            // Then map the middle 2m pages.
            //
            AddressMapping body = detail::AlignLargeRangeBody(mapping);
            mapRange2m(body, flags, type, buffer);

            //
            // Finally map the trailing 4k pages.
            //
            AddressMapping tail = detail::AlignLargeRangeTail(mapping);
            if (!tail.isEmpty()) {
                mapRange4k(tail, flags, type, buffer);
            }

            return;
        }
    }

    //
    // If we get to this point its not worth using 2m pages
    // so we map the range with 4k pages.
    //
    mapRange4k(mapping, flags, type, buffer);
}

bool PageTables::verifyMapping(AddressMapping mapping) const noexcept [[clang::nonblocking]] {
    bool valid = (mapping.size > 0)
        && (mapping.size % x64::kPageSize == 0)
        && (mapping.paddr.address % x64::kPageSize == 0)
        && ((uintptr_t)mapping.vaddr % x64::kPageSize == 0)
        && mPageManager->isCanonicalAddress(mapping.vaddr);

    return valid;
}

OsStatus PageTables::addBackingMemory(AddressMapping mapping) noexcept [[clang::nonallocating]] {
    return mCache.addMemory(mapping);
}

OsStatus PageTables::reclaimBackingMemory(AddressMapping *mapping [[outparam]]) noexcept [[clang::nonallocating]] {

}

OsStatus PageTables::map(AddressMapping mapping, PageFlags flags, MemoryType type) {
    PageMappingRequest request;
    if (OsStatus status = PageMappingRequest::create(mapping, flags, type, &request)) [[unlikely]] {
        return status;
    }

    return map(request);
}

OsStatus PageTables::map(const PageMappingRequest& request) {
    km::AddressMapping mapping = request.mapping();
    km::PageFlags flags = request.flags();
    km::MemoryType type = request.type();

    detail::PageTableList buffer;
    if (OsStatus status = allocatePageTables(mapping.virtualRange(), &buffer)) {
        return status;
    }

    //
    // Once we are done we need to deallocate the unused tables.
    //
    defer { drainTableList(std::move(buffer)); };

    mapWithList(mapping, flags, type, buffer);

    for (sm::VirtualAddress i = mapping.vaddr; i < sm::VirtualAddress(mapping.vaddr) + mapping.size; i += x64::kPageSize) {
        KM_CHECK(getPageSize(i) != PageSize::eNone, "Mapping failed to map the page.");
    }

    return OsStatusSuccess;
}

PageTableStats PageTables::stats() const noexcept [[clang::nonallocating]] {
    PageTableStats stats{};
    stats.allocatorStats = mAllocator.stats();

    const x64::PageMapLevel4 *root = pml4();

    for (size_t i = 0; i < 512; i++) {
        if (!root->entries[i].present()) {
            continue;
        }

        stats.pml4Entries += 1;

        for (size_t j = 0; j < 512; j++) {
            x64::PageMapLevel3 *l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(root->entries[i]));
            x64::pdpte& pdpte = l3->entries[j];
            if (!pdpte.present()) {
                continue;
            }

            stats.pdptEntries += 1;

            if (pdpte.is1g()) {
                continue;
            }

            for (size_t k = 0; k < 512; k++) {
                x64::PageMapLevel2 *pdpte = asVirtual<x64::PageMapLevel2>(mPageManager->address(root->entries[i]));
                x64::pdte& pdte = pdpte->entries[k];
                if (!pdte.present()) {
                    continue;
                }

                stats.pdEntries += 1;

                if (pdte.is2m()) {
                    continue;
                }

                for (size_t l = 0; l < 512; l++) {
                    x64::PageTable *pdte = asVirtual<x64::PageTable>(mPageManager->address(root->entries[i]));
                    x64::pte& pte = pdte->entries[l];
                    if (pte.present()) {
                        stats.ptEntries += 1;
                    }
                }
            }
        }
    }

    return stats;
}

void PageTables::partialRemap2m(x64::PageTable *pt, VirtualRange range, PhysicalAddressEx paddr, MemoryType type) {
    KM_CHECK(!range.isEmpty(), "Cannot remap empty range.");

    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back; i += x64::kPageSize) {
        auto [pml4e, pdpte, pdte, pte] = getAddressParts(i);
        x64::pte& t1 = pt->entries[pte];
        mPageManager->setMemoryType(t1, type);
        setEntryFlags(t1, mMiddleFlags, paddr + (i - (uintptr_t)range.front));
    }
}

void PageTables::split2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase, PageTableAllocation allocation) {
    KM_CHECK(pde.is2m(), "Splitting pde that is already split.");
    x64::PageTable *pt = std::bit_cast<x64::PageTable*>(allocation.getVirtual());

    auto [lo, hi] = km::split(page, erase);
    MemoryType type = mPageManager->getMemoryType(pde);
    PhysicalAddressEx paddr = mPageManager->address(pde);
    uintptr_t hiOffset = (uintptr_t)erase.back - (uintptr_t)page.front;

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
    setEntryFlags(pde, mMiddleFlags, asPhysical(allocation));
    pde.set2m(false);
}

void PageTables::cut2mMapping(x64::pdte& pde, VirtualRange page, VirtualRange erase, PageTableAllocation allocation) {
    VirtualRange remaining = page.cut(erase);
    x64::PageTable *pt = std::bit_cast<x64::PageTable*>(allocation.getVirtual());

    MemoryType type = mPageManager->getMemoryType(pde);
    PhysicalAddressEx paddr = mPageManager->address(pde);
    uintptr_t offset = (uintptr_t)remaining.front - (uintptr_t)page.front;

    //
    // Remap the area that is still valid.
    //
    partialRemap2m(pt, remaining, paddr + offset, type);

    //
    // Update the 2m page to point to the new 4k page table.
    //
    setEntryFlags(pde, mMiddleFlags, asPhysical(allocation));
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
        sm::PhysicalAddress paddr = mPageManager->address(pde);
        x64::PageTable *pt = asVirtual<x64::PageTable>(paddr);
        deallocate(pt, paddr, 1);
    }

    pde.setPresent(false);
    pde.set2m(false);
}

int PageTables::countRequiredPageTables(VirtualRange range) noexcept [[clang::nonallocating]] {
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

x64::pdte& PageTables::getLargePageEntry(const void *address) noexcept [[clang::nonallocating]] {
    auto [pml4e, pdpte, pdte, _] = getAddressParts(address);

    x64::PageMapLevel4 *l4 = pml4();

    x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    KM_CHECK(l3 != nullptr, "Failed to find page map level 3.");

    x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    KM_CHECK(l2 != nullptr, "Failed to find page map level 2.");

    return l2->entries[pdte];
}

PageWalk PageTables::walkUnlocked(sm::VirtualAddress ptr) const noexcept [[clang::nonblocking]] {
    auto [pml4eIndex, pdpteIndex, pdteIndex, pteIndex] = getAddressParts(ptr);
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

void PageTables::earlyUnmapWithList(int earlyAllocations, VirtualRange range, VirtualRange *remaining, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    if (earlyAllocations == 2) {
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

        cut2mMapping(lowEntry, lowPage, low, buffer.next());
        cut2mMapping(highEntry, highPage, high, buffer.next());

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
            split2mMapping(entry, page, range, buffer.next());
            *remaining = VirtualRange{};
            return;
        }

        //
        // If we get here we know that the range is not entirely contained within a 2m page.
        // So we need to allocate early in case the unmap ends partially in the last 2m page.
        //

        if (page.front != range.front) {
            x64::pdte& entry = getLargePageEntry(range.front);
            cut2mMapping(entry, page, range, buffer.next());
        } else {
            KM_CHECK(page.back != range.back, "Invalid range.");
            x64::pdte& entry = getLargePageEntry(range.front);
            cut2mMapping(entry, page, range, buffer.next());
        }

        *remaining = VirtualRange{};
    }
}

OsStatus PageTables::earlyUnmap(VirtualRange range, VirtualRange *remaining) {
    //
    // Fun edge case when unampping a range of memory that isnt 2m aligned but intersects with 2 2m pages.
    // As this requires allocating 2 new page tables to store the 4k mappings required: the first allocation
    // can succeed and the second can fail. If we did the allocating while iterating we could reach a state
    // where the second allocation fails but page tables have already been manipulated. So we need to allocate both
    // required page tables upfront to ensure both exist before unmapping the remaining area.
    //
    int earlyAllocations = countRequiredPageTables(range);
    if (earlyAllocations == 0) {
        return OsStatusSuccess;
    }

    detail::PageTableList buffer;
    if (!mAllocator.allocateList(earlyAllocations, &buffer)) {
        return OsStatusOutOfMemory;
    }

    earlyUnmapWithList(earlyAllocations, range, remaining, buffer);

    return OsStatusSuccess;
}

void PageTables::unmapWithList(VirtualRange range, detail::PageTableList& buffer) noexcept [[clang::nonallocating]] {
    int earlyAllocations = countRequiredPageTables(range);
    if (earlyAllocations != 0) {
        earlyUnmapWithList(earlyAllocations, range, &range, buffer);
    }

    unmapUnlocked(range);
}

void PageTables::unmapUnlocked(VirtualRange range) noexcept [[clang::nonallocating]] {
    x64::PageMapLevel4 *l4 = pml4();

    uintptr_t i = (uintptr_t)range.front;
    uintptr_t end = (uintptr_t)range.back;

    //
    // Iterate over the range we want to unmap and unmap the pages.
    //
    while (i < end) {
        auto [pml4e, pdpte, pdte, pte] = getAddressParts(i);

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
            reclaim2m(pde);
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
}

OsStatus PageTables::unmap(VirtualRange range) {
    range = alignedOut(range, x64::kPageSize);

    if (OsStatus status = earlyUnmap(range, &range)) {
        return status;
    }

    unmapUnlocked(range);

    return OsStatusSuccess;
}

OsStatus PageTables::unmap2m(VirtualRange range) {
    //
    // Test preconditions to ensure the range is valid for 2m unmap.
    //
    bool valid = ((uintptr_t)range.front % x64::kLargePageSize == 0)
        && ((uintptr_t)range.back % x64::kLargePageSize == 0)
        && range.size() >= x64::kLargePageSize;

    if (!valid) [[unlikely]] {
        return OsStatusInvalidInput;
    }

    x64::PageMapLevel4 *l4 = pml4();

    for (uintptr_t i = (uintptr_t)range.front; i < (uintptr_t)range.back;) {
        auto [pml4e, pdpte, pdte, _] = getAddressParts(i);

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

PhysicalAddressEx PageTables::getBackingAddressUnlocked(const void *ptr) const {
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    auto [pml4e, pdpte, pdte, pte] = getAddressParts(ptr);
    uintptr_t offset = address & 0xFFF;

    const x64::PageMapLevel4 *l4 = pml4();
    const x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) return PhysicalAddressEx::invalid();

    const x64::PageMapLevel3 *l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) return PhysicalAddressEx::invalid();

    if (t3.is1g()) {
        uintptr_t pdpteOffset = address & 0x3FF'000;
        return km::PhysicalAddressEx { mPageManager->address(t3) + pdpteOffset + offset };
    }

    const x64::PageMapLevel2 *l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    const x64::pdte& t2 = l2->entries[pdte];
    if (!t2.present()) return PhysicalAddressEx::invalid();

    if (t2.is2m()) {
        uintptr_t pdteOffset = address & 0x1FF'000;
        return km::PhysicalAddressEx { mPageManager->address(t2) + pdteOffset + offset };
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return PhysicalAddressEx::invalid();

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return PhysicalAddressEx::invalid();

    return km::PhysicalAddressEx { mPageManager->address(t1) + offset };
}

km::PhysicalAddressEx PageTables::getBackingAddress(const void *ptr) {
    return getBackingAddressUnlocked(ptr);
}

km::PageFlags PageTables::getMemoryFlags(const void *ptr) {
    PageWalk result = walk(ptr);
    return result.flags();
}

km::PageSize PageTables::getPageSize(const void *ptr) {
    PageWalk result = walk(ptr);
    return result.pageSize();
}

PageWalk PageTables::walk(const void *ptr) {
    return walkUnlocked(ptr);
}

template<typename T>
static bool IsTableEmpty(const T *pt) {
    for (const auto& entry : pt->entries) {
        if (entry.present()) {
            return false;
        }
    }

    return true;
}

size_t PageTables::compactPt(x64::PageMapLevel2 *pd, uint16_t index) {
    auto& entry = pd->entries[index];
    if (!entry.present()) return 0; // Is not mapped, nothing to do.
    if (entry.is2m()) return 0; // Is mapped.

    sm::PhysicalAddress paddr = mPageManager->address(entry);
    x64::PageTable *pt = asVirtual<x64::PageTable>(paddr);
    if (IsTableEmpty(pt)) {
        deallocate(pt, paddr, 1);
        entry.setPresent(false);
        return 1;
    }

    return 0;
}

size_t PageTables::compactPd(x64::PageMapLevel3 *pd, uint16_t index) {
    auto& entry = pd->entries[index];
    if (!entry.present()) return 0;
    if (entry.is1g()) return 0;

    sm::PhysicalAddress paddr = mPageManager->address(entry);
    x64::PageMapLevel2 *pt = asVirtual<x64::PageMapLevel2>(paddr);
    size_t count = 0;
    bool reclaim = true;
    for (size_t i = 0; i < 512; i++) {
        count += compactPt(pt, i);

        if (pt->entries[i].present()) {
            reclaim = false;
        }
    }

    if (reclaim) {
        deallocate(pt, paddr, 1);
        entry.setPresent(false);
        count += 1;
    }

    return count;
}

size_t PageTables::compactPdpt(x64::PageMapLevel4 *pd, uint16_t index) {
    auto& entry = pd->entries[index];
    if (!entry.present()) return 0;
    size_t count = 0;

    sm::PhysicalAddress paddr = mPageManager->address(entry);
    x64::PageMapLevel3 *pt = asVirtual<x64::PageMapLevel3>(paddr);
    bool reclaim = true;
    for (size_t i = 0; i < 512; i++) {
        count += compactPd(pt, i);

        if (pt->entries[i].present()) {
            reclaim = false;
        }
    }

    if (reclaim) {
        deallocate(pt, paddr, 1);
        entry.setPresent(false);
        count += 1;
    }

    return count;
}

size_t PageTables::compactPml4(x64::PageMapLevel4 *pml4) {
    size_t count = 0;
    // Don't check to see if we can compact the pml4, we always want to keep it.
    for (size_t i = 0; i < 512; i++) {
        count += compactPdpt(pml4, i);
    }
    return count;
}

size_t PageTables::compactUnlocked() {
    x64::PageMapLevel4 *l4 = pml4();
    return compactPml4(l4);
}

size_t PageTables::compact() {
    return compactUnlocked();
}
