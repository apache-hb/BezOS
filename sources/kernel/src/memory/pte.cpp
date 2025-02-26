#include "memory/pte.hpp"

#include "log.hpp"

#include "arch/paging.hpp"
#include "memory/paging.hpp"

#include <limits.h>
#include <string.h>

#include <bit>

using namespace km;

x64::page *PageTableManager::alloc4k() {
    if (x64::page *page = mAllocator.construct<x64::page>()) {
        memset(page, 0, sizeof(x64::page));

        return page;
    }

    KM_ASSERT("Failed to allocate page table.");

    return nullptr;
}

void PageTableManager::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
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

PageTableManager::PageTableManager(const km::PageBuilder *pm, AddressMapping pteMemory)
    : mSlide(pteMemory.slide())
    , mPageManager(pm)
    , mAllocator((void*)pteMemory.vaddr, pteMemory.size)
    , mRootPageTable((x64::PageMapLevel4*)alloc4k())
{ }

x64::PageMapLevel3 *PageTableManager::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e, PageFlags flags) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
        if (l3) setEntryFlags(t4, flags, asPhysical(l3));
    } else {
        l3 = asVirtual<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *PageTableManager::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte, PageFlags flags) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
        if (l2) setEntryFlags(t3, flags, asPhysical(l2));
    } else {
        l2 = asVirtual<x64::PageMapLevel2>(mPageManager->address(t3));
    }

    return l2;
}

const x64::PageMapLevel3 *PageTableManager::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return asVirtual<x64::PageMapLevel3>(base);
    }

    return nullptr;
}

const x64::PageMapLevel2 *PageTableManager::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const {
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (t3.present() && !t3.is1g()) {
        uintptr_t base = mPageManager->address(t3);
        return asVirtual<x64::PageMapLevel2>(base);
    }

    return nullptr;
}

x64::PageTable *PageTableManager::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const {
    const x64::pdte& pde = l2->entries[pdte];
    if (pde.present() && !pde.is2m()) {
        uintptr_t base = mPageManager->address(pde);
        return asVirtual<x64::PageTable>(base);
    }

    return nullptr;
}

OsStatus PageTableManager::mapRange4k(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kPageSize) {
        if (OsStatus status = map4k(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus PageTableManager::mapRange2m(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kLargePageSize) {
        if (OsStatus status = map2m(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus PageTableManager::mapRange1g(AddressMapping mapping, PageFlags flags, MemoryType type) {
    for (size_t i = 0; i < mapping.size; i += x64::kHugePageSize) {
        if (OsStatus status = map1g(mapping.paddr + i, (char*)mapping.vaddr + i, flags, type)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus PageTableManager::map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
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
        pt = std::bit_cast<x64::PageTable*>(alloc4k());
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

OsStatus PageTableManager::map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
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

OsStatus PageTableManager::map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
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

static bool IsLargePageAligned(PhysicalAddress paddr, const void *vaddr) {
    uintptr_t addr = (uintptr_t)vaddr;
    return (paddr.address & (x64::kLargePageSize - 1)) == 0 && (addr & (x64::kLargePageSize - 1)) == 0;
}

void PageTableManager::map(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    // align everything to 4k page boundaries
    range.front = sm::rounddown(range.front.address, x64::kPageSize);
    range.back = sm::roundup(range.back.address, x64::kPageSize);

    // check if we should we attempt to use large pages
    if ((uintptr_t)range.size() >= x64::kLargePageSize && IsLargePageAligned(range.front, vaddr)) {
        // if the range is larger than 2m in total, check if
        // the range is still larger than 2m after aligning the range.
        PhysicalAddress front2m = sm::roundup(range.front.address, x64::kLargePageSize);
        PhysicalAddress back2m = sm::rounddown(range.back.address, x64::kLargePageSize);

        // if the range is still larger than 2m, we can use 2m pages
        if (front2m < back2m) {
            // map the leading 4k pages we need to map to fulfill our api contract
            MemoryRange head = {range.front, front2m};
            if (!head.isEmpty())
                mapRange4k(MappingOf(head, vaddr), flags, type);

            // then map the 2m pages
            MemoryRange body = {front2m, back2m};
            mapRange2m(MappingOf(body, (char*)vaddr + head.size()), flags, type);

            // finally map the trailing 4k pages
            MemoryRange tail = {back2m, range.back};
            if (!tail.isEmpty())
                mapRange4k(MappingOf(tail, (char*)vaddr + head.size() + body.size()), flags, type);

            return;
        }
    }

    // if we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages
    mapRange4k(MappingOf(range, vaddr), flags, type);
}

void PageTableManager::unmap(void *ptr, size_t size) {
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

km::PhysicalAddress PageTableManager::getBackingAddress(const void *ptr) {
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

km::PageFlags PageTableManager::getMemoryFlags(const void *ptr) {
    PageWalk result;
    if (walk(ptr, &result) != OsStatusSuccess) {
        return PageFlags::eNone;
    }

    return result.flags();
}

km::PageSize2 PageTableManager::getPageSize(const void *ptr) {
    PageWalk result;
    if (walk(ptr, &result) != OsStatusSuccess) {
        return PageSize2::eNone;
    }

    return result.pageSize();
}

km::PhysicalAddress PageTableManager::asPhysical(const void *ptr) const {
    return km::PhysicalAddress { (uintptr_t)ptr - mSlide };
}

OsStatus PageTableManager::walk(const void *ptr, PageWalk *walk) {
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
