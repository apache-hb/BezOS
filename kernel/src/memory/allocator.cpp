#include "memory/allocator.hpp"

#include "arch/paging.hpp"
#include "memory/paging.hpp"

#include "kernel.hpp"

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
    PhysicalAddress result = mPageAllocator->alloc4k();
    uintptr_t offset = (uintptr_t)result.address + mPageManager->hhdmOffset();
    x64::page *page = (x64::page*)offset;
    memset(page, 0, sizeof(x64::page));
    return page;
}

void PageTableManager::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));

    entry.setPresent(true);
}

PageTableManager::PageTableManager(const km::PageBuilder *pm, km::VirtualAllocator *vmm, PageAllocator *alloc)
    : mPageManager(pm)
    , mPageAllocator(alloc)
    , mVirtualAllocator(vmm)
    , mRootPageTable(alloc4k())
{ }

x64::PageMapLevel3 *PageTableManager::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e) {
    x64::PageMapLevel3 *l3;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
        setEntryFlags(t4, PageFlags::eAll, (uintptr_t)l3 - mPageManager->hhdmOffset());
    } else {
        l3 = std::bit_cast<x64::PageMapLevel3*>(mPageManager->address(t4) + mPageManager->hhdmOffset());
    }

    return l3;
}

x64::PageMapLevel2 *PageTableManager::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte) {
    x64::PageMapLevel2 *l2;

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
        setEntryFlags(t3, PageFlags::eAll, (uintptr_t)l2 - mPageManager->hhdmOffset());
    } else {
        l2 = std::bit_cast<x64::PageMapLevel2*>(mPageManager->address(t3) + mPageManager->hhdmOffset());
    }

    return l2;
}

const x64::PageMapLevel3 *PageTableManager::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return std::bit_cast<x64::PageMapLevel3*>(base + mPageManager->hhdmOffset());
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
        return std::bit_cast<x64::PageMapLevel2*>(base + mPageManager->hhdmOffset());
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
        return std::bit_cast<x64::PageTable*>(base + mPageManager->hhdmOffset());
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

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);
    x64::PageTable *pt;

    x64::pde& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(alloc4k());
        setEntryFlags(t2, PageFlags::eAll, (uintptr_t)pt - mPageManager->hhdmOffset());
    } else {
        pt = std::bit_cast<x64::PageTable*>(mPageManager->address(t2) + mPageManager->hhdmOffset());
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

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
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

    // if we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages
    mapRange4k(range, vaddr, flags, type);
}

void PageTableManager::unmap(void *ptr, size_t size) {
    uintptr_t front = sm::rounddown((uintptr_t)ptr, x64::kPageSize);
    uintptr_t back = sm::roundup((uintptr_t)ptr + size, x64::kPageSize);

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();

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

static void MapStage1Memory(PageTableManager& memory, const km::PageBuilder& pm, const SystemMemoryLayout& layout) {
    for (MemoryMapEntry range : layout.available) {
        memory.mapRange(range.range, (void*)(range.range.front.address + pm.hhdmOffset()), PageFlags::eData, MemoryType::eWriteBack);
    }

    for (MemoryMapEntry range : layout.reclaimable) {
        memory.mapRange(range.range, (void*)(range.range.front.address + pm.hhdmOffset()), PageFlags::eData, MemoryType::eWriteBack);
    }
}

void KmMapKernel(const km::PageBuilder& pm, km::PageTableManager& vmm, km::SystemMemoryLayout& layout, km::PhysicalAddress paddr, const void *vaddr) {
    // first map the kernel pages
    MapKernelPages(vmm, paddr, vaddr);

    // then remap the rest of memory to the higher half
    MapStage1Memory(vmm, pm, layout);
}

void KmMigrateMemory(km::PageTableManager& vmm, km::PageBuilder& pm, km::PhysicalAddress base, size_t size, km::MemoryType type) {
    km::PhysicalAddress end = base + size;

    vmm.mapRange({ base, end }, (void*)(base.address + pm.hhdmOffset()), PageFlags::eData, type);
}

void KmReclaimBootMemory(const km::PageBuilder& pm, km::PageTableManager& vmm, km::SystemMemoryLayout& layout) {
    KmDebugMessage("[INIT] Moving to kernel pagemap.\n");

    // then apply the new page tables
    pm.setActiveMap((uintptr_t)vmm.rootPageTable() - pm.hhdmOffset());

    KmDebugMessage("[INIT] Reclaiming bootloader memory.\n");

    layout.reclaimBootMemory();

    KmDebugMessage("[INIT] Bootloader memory reclaimed.\n");
}
