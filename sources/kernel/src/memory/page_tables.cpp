#include "log.hpp"
#include "memory/allocator.hpp"

#include "arch/paging.hpp"
#include "memory/paging.hpp"

#include <limits.h>
#include <string.h>

#include <bit>

using namespace km;

x64::page *PageTableManager::alloc4k() {
    if (x64::page *page = mAllocator->allocate<x64::page>()) {
        memset(page, 0, sizeof(x64::page));

        return page;
    }

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

PageTableManager::PageTableManager(const km::PageBuilder *pm, mem::IAllocator *allocator)
    : mPageManager(pm)
    , mAllocator(allocator)
    , mRootPageTable((x64::PageMapLevel4*)alloc4k())
{ }

x64::PageMapLevel3 *PageTableManager::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
        setEntryFlags(t4, PageFlags::eAllUser, getPhysicalAddress(l3));
    } else {
        l3 = getVirtualAddress<x64::PageMapLevel3>(mPageManager->address(t4));
    }

    return l3;
}

x64::PageMapLevel2 *PageTableManager::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
        setEntryFlags(t3, PageFlags::eAllUser, getPhysicalAddress(l2));
    } else {
        l2 = getVirtualAddress<x64::PageMapLevel2>(mPageManager->address(t3));
    }

    return l2;
}

const x64::PageMapLevel3 *PageTableManager::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return getVirtualAddress<x64::PageMapLevel3>(base);
    }

    return nullptr;
}

const x64::PageMapLevel2 *PageTableManager::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const {
    const x64::pdpte& t3 = l3->entries[pdpte];
    if (t3.present() && !t3.is1g()) {
        uintptr_t base = mPageManager->address(t3);
        return getVirtualAddress<x64::PageMapLevel2>(base);
    }

    return nullptr;
}

x64::PageTable *PageTableManager::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const {
    const x64::pde& pde = l2->entries[pdte];
    if (pde.present() && !pde.is2m()) {
        uintptr_t base = mPageManager->address(pde);
        return getVirtualAddress<x64::PageTable>(base);
    }

    return nullptr;
}

void PageTableManager::mapRange4k(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
        map4k(i, vaddr, flags, type);
        vaddr = (char*)vaddr + x64::kPageSize;
    }
}

void PageTableManager::mapRange2m(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kLargePageSize) {
        map2m(i, vaddr, flags, type);
        vaddr = (char*)vaddr + x64::kLargePageSize;
    }
}

void PageTableManager::mapRange1g(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kHugePageSize) {
        map1g(i, vaddr, flags, type);
        vaddr = (char*)vaddr + x64::kHugePageSize;
    }
}

void PageTableManager::map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
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
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);
    x64::PageTable *pt;

    x64::pde& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(alloc4k());
        setEntryFlags(t2, PageFlags::eAllUser, getPhysicalAddress(pt));
    } else {
        pt = getVirtualAddress<x64::PageTable>(mPageManager->address(t2));
    }

    x64::pte& t1 = pt->entries[pte];
    mPageManager->setMemoryType(t1, type);
    setEntryFlags(t1, flags, paddr);
}

void PageTableManager::map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
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
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);

    x64::pde& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr);
}

void PageTableManager::map1g(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    stdx::LockGuard guard(mLock);

    uintptr_t addr = (uintptr_t)vaddr;
    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);

    x64::pdpte& t3 = l3->entries[pdpte];
    t3.set1g(true);
    mPageManager->setMemoryType(t3, type);
    setEntryFlags(t3, flags, paddr);
}

static bool IsLargePageAligned(PhysicalAddress paddr, const void *vaddr) {
    uintptr_t addr = (uintptr_t)vaddr;
    return (paddr.address & (x64::kLargePageSize - 1)) == 0 && (addr & (x64::kLargePageSize - 1)) == 0;
}

void PageTableManager::mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
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
                mapRange4k(head, vaddr, flags, type);

            // then map the 2m pages
            MemoryRange body = {front2m, back2m};
            mapRange2m(body, (char*)vaddr + head.size(), flags, type);

            // finally map the trailing 4k pages
            MemoryRange tail = {back2m, range.back};
            if (!tail.isEmpty())
                mapRange4k(tail, (char*)vaddr + head.size() + body.size(), flags, type);

            return;
        }
    }

    // if we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages
    mapRange4k(range, vaddr, flags, type);
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

km::PhysicalAddress PageTableManager::getBackingAddress(const void *ptr) const {
    stdx::LockGuard guard(mLock);

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;
    uintptr_t offset = address & 0xFFF;

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return nullptr;

    if (l3->entries[pdpte].is1g()) {
        uintptr_t pdpteOffset = address & 0x3FF'000;
        return km::PhysicalAddress { mPageManager->address(l3->entries[pdpte]) + pdpteOffset + offset };
    }

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return nullptr;

    if (l2->entries[pdte].is2m()) {
        uintptr_t pdteOffset = address & 0x1FF'000;
        return km::PhysicalAddress { mPageManager->address(l2->entries[pdte]) + pdteOffset + offset };
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return nullptr;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return nullptr;

    return km::PhysicalAddress { mPageManager->address(t1) + offset };
}

PageFlags PageTableManager::getMemoryFlags(const void *ptr) const {
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

PageSize PageTableManager::getPageSize(const void *ptr) const {
    stdx::LockGuard guard(mLock);

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return PageSize::eNone;

    if (l3->entries[pdpte].is1g()) {
        return PageSize::eHuge;
    }

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return PageSize::eNone;

    if (l2->entries[pdte].is2m()) {
        return PageSize::eLarge;
    }

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return PageSize::eNone;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return PageSize::eNone;

    return PageSize::eDefault;
}
