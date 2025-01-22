#include "memory/allocator.hpp"

#include "arch/paging.hpp"
#include "log.hpp"
#include "memory/paging.hpp"

#include <limits.h>
#include <string.h>

#include <bit>

using namespace km;

extern "C" {
    extern char __text_start[]; // always aligned to 0x1000
    extern char __text_end[];

    extern char __rodata_start[]; // always aligned to 0x1000
    extern char __rodata_end[];

    extern char __data_start[]; // always aligned to 0x1000
    extern char __data_end[];

    extern char __kernel_start[];
    extern char __kernel_end[];
}

x64::page *PageTableManager::alloc4k() {
    x64::page *page = (x64::page*)mAllocator->allocateAligned(x64::kPageSize, x64::kPageSize);
    memset(page, 0, sizeof(x64::page));
    return page;
}

void PageTableManager::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
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
    if (t3.present()) {
        if (t3.is1g()) {
            KmDebugMessage("[WARN] Attempted to unmap a 1g page. Currently unimplemented, sorry :(\n");
            return nullptr;
        }

        uintptr_t base = mPageManager->address(t3);
        return getVirtualAddress<x64::PageMapLevel2>(base);
    }

    return nullptr;
}

x64::PageTable *PageTableManager::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const {
    const x64::pde& pde = l2->entries[pdte];
    if (pde.present()) {
        if (pde.is2m()) {
            KmDebugMessage("[WARN] Attempted to unmap a 2m page. Currently unimplemented, sorry :(\n");
            return nullptr;
        }

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

void PageTableManager::map4k(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    uintptr_t addr = (uintptr_t)vaddr;
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
    setEntryFlags(t1, flags, paddr.address);
}

void PageTableManager::map2m(PhysicalAddress paddr, const void *vaddr, PageFlags flags, MemoryType type) {
    uintptr_t addr = (uintptr_t)vaddr;
    uint16_t pml4e = (addr >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (addr >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (addr >> 21) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = getRootTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);

    x64::pde& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr.address);
}

void PageTableManager::mapRange(MemoryRange range, const void *vaddr, PageFlags flags, MemoryType type) {
    // align everything to 4k page boundaries
    range.front = sm::rounddown(range.front.address, x64::kPageSize);
    range.back = sm::roundup(range.back.address, x64::kPageSize);

#if 0
    // check if we should we attempt to use large pages
    if ((uintptr_t)range.size() >= x64::kLargePageSize) {
        // if the range is larger than 2m in total, check if
        // the range is still larger than 2m after aligning the range.
        PhysicalAddress front2m = sm::roundup(range.front.address, x64::kLargePageSize);
        PhysicalAddress back2m = sm::rounddown(range.back.address, x64::kLargePageSize);

        // if the range is still larger than 2m, we can use 2m pages
        if (front2m < back2m) {
            // map the leading 4k pages we need to map to fulfill our api contract
            MemoryRange head = {range.front, front2m};
            mapRange4k(head, vaddr, flags, type);

            // then map the 2m pages
            MemoryRange body = {front2m, back2m};
            mapRange2m(body, (char*)vaddr + head.size(), flags, type);

            // finally map the trailing 4k pages
            MemoryRange tail = {back2m, range.back};
            mapRange4k(tail, (char*)vaddr + head.size() + body.size(), flags, type);
            return;
        }
    }
#endif

    // if we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages
    mapRange4k(range, vaddr, flags, type);
}

void PageTableManager::unmap(void *ptr, size_t size) {
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
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t pml4e = (address >> 39) & 0b0001'1111'1111;
    uintptr_t pdpte = (address >> 30) & 0b0001'1111'1111;
    uintptr_t pdte = (address >> 21) & 0b0001'1111'1111;
    uintptr_t pte = (address >> 12) & 0b0001'1111'1111;
    uintptr_t offset = address & 0xFFF;

    const x64::PageMapLevel4 *l4 = getRootTable();
    const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
    if (!l3) return nullptr;

    const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
    if (!l2) return nullptr;

    const x64::PageTable *pt = findPageTable(l2, pdte);
    if (!pt) return nullptr;

    const x64::pte& t1 = pt->entries[pte];
    if (!t1.present()) return nullptr;

    return km::PhysicalAddress { mPageManager->address(t1) + offset };
}

static void MapKernelPages(PageTableManager& memory, km::PhysicalAddress paddr, const void *vaddr) {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags, stdx::StringView name) {
        km::PhysicalAddress front = km::PhysicalAddress {  (uintptr_t)begin - (uintptr_t)__kernel_start };
        km::PhysicalAddress back = km::PhysicalAddress { (uintptr_t)end - (uintptr_t)__kernel_start };

        KmDebugMessage("[INIT] Mapping ", Hex((uintptr_t)begin), "-", Hex((uintptr_t)end), " - ", name, "\n");

        memory.mapRange({ paddr + front.address, paddr + back.address }, (char*)vaddr + front.address, flags, MemoryType::eWriteBack);
    };

    KmDebugMessage("[INIT] Mapping kernel pages.\n");

    mapKernelRange(__text_start, __text_end, PageFlags::eCode, ".text");
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead, ".rodata");
    mapKernelRange(__data_start, __data_end, PageFlags::eData, ".data");
}

void KmMapKernel(km::PageTableManager& vmm, km::PhysicalAddress paddr, const void *vaddr) {
    // first map the kernel pages
    MapKernelPages(vmm, paddr, vaddr);
}

void KmMigrateMemory(km::PageTableManager& vmm, km::PageBuilder& pm, km::PhysicalAddress base, size_t size, km::MemoryType type) {
    KmMapMemory(vmm, base, (void*)(base.address + pm.hhdmOffset()), size, PageFlags::eData, type);
}

void KmMapMemory(km::PageTableManager& vmm, km::PhysicalAddress base, const void *vaddr, size_t size, km::PageFlags flags, km::MemoryType type) {
    km::PhysicalAddress end = base + size;

    vmm.mapRange({ base, end }, vaddr, flags, type);
}

void KmReclaimBootMemory(const km::PageBuilder& pm, km::PageTableManager& vmm) {
    // then apply the new page tables
    KmDebugMessage("[INIT] PML4: ", vmm.rootPageTable(), "\n");
    pm.setActiveMap(vmm.rootPageTable());
}
