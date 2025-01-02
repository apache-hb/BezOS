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

/// page allocator

static size_t GetBitmapSize(const SystemMemoryLayout *layout) {
    size_t size = 0;

    for (MemoryMapEntry entry : layout->available) {
        size += detail::GetRangeBitmapSize(entry.range);
    }

    for (MemoryMapEntry entry : layout->reclaimable) {
        size += detail::GetRangeBitmapSize(entry.range);
    }

    return size;
}

static PhysicalAddress FindBitmapSpace(const SystemMemoryLayout *layout, size_t bitmapSize) {
    // Find any memory range that is large enough to hold the bitmap
    // and is not in the first 1MB of memory.

    for (MemoryMapEntry entry : layout->available) {
        if (entry.range.size() >= bitmapSize && !entry.range.contains(0x100000)) {
            return entry.range.front;
        }
    }

    return nullptr;
}

PageAllocator::PageAllocator(const SystemMemoryLayout *layout, uintptr_t hhdmOffset) {
    // Get the size of the bitmap and round up to the nearest page.
    size_t bitmapSize = sm::roundup(GetBitmapSize(layout), x64::kPageSize);

    // Allocate bitmap space
    PhysicalAddress bitmap = FindBitmapSpace(layout, bitmapSize);

    // Create an allocator for each memory range
    size_t offset = 0;
    for (MemoryMapEntry entry : layout->available) {
        RegionBitmapAllocator allocator{entry.range, (uint8_t*)(bitmap + offset + hhdmOffset).address};
        if (entry.range.front < 0x100000) {
            mLowMemory.add(allocator);
        } else {
            mAllocators.add(allocator);
        }

        offset += detail::GetRangeBitmapSize(entry.range);
    }

    // Mark the bitmap memory as used
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (allocator.contains(bitmap)) {
            allocator.markAsUsed({bitmap, bitmap + bitmapSize});
            break;
        }
    }
}

PhysicalAddress PageAllocator::alloc4k(size_t count) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (PhysicalAddress addr = allocator.alloc4k(count); addr != nullptr) {
            return addr;
        }
    }

    return nullptr;
}

PhysicalAddress PageAllocator::lowMemoryAlloc4k() {
    for (RegionBitmapAllocator& allocator : mLowMemory) {
        if (PhysicalAddress addr = allocator.alloc4k(1); addr != nullptr) {
            return addr;
        }
    }

    return nullptr;
}

void PageAllocator::markRangeUsed(MemoryRange range) {
    for (RegionBitmapAllocator& allocator : mAllocators) {
        if (allocator.contains(range.front)) {
            allocator.markAsUsed(range);
        }
    }

    for (RegionBitmapAllocator& allocator : mLowMemory) {
        if (allocator.contains(range.front)) {
            allocator.markAsUsed(range);
        }
    }
}

x64::page *VirtualAllocator::alloc4k() {
    PhysicalAddress result = mPageAllocator->alloc4k();
    uintptr_t offset = (uintptr_t)result.address + mPageManager->hhdmOffset();
    x64::page *page = (x64::page*)offset;
    memset(page, 0, sizeof(x64::page));
    return page;
}

void VirtualAllocator::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));

    entry.setPresent(true);
}

VirtualAllocator::VirtualAllocator(const km::PageManager *pm, PageAllocator *alloc)
    : mPageManager(pm)
    , mPageAllocator(alloc)
    , mRootPageTable(alloc4k())
{ }

x64::PageMapLevel3 *VirtualAllocator::getPageMap3(x64::PageMapLevel4 *l4, uint16_t pml4e) {
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

x64::PageMapLevel2 *VirtualAllocator::getPageMap2(x64::PageMapLevel3 *l3, uint16_t pdpte) {
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

const x64::PageMapLevel3 *VirtualAllocator::findPageMap3(const x64::PageMapLevel4 *l4, uint16_t pml4e) const {
    if (l4->entries[pml4e].present()) {
        uintptr_t base = mPageManager->address(l4->entries[pml4e]);
        return std::bit_cast<x64::PageMapLevel3*>(base + mPageManager->hhdmOffset());
    }

    return nullptr;
}

const x64::PageMapLevel2 *VirtualAllocator::findPageMap2(const x64::PageMapLevel3 *l3, uint16_t pdpte) const {
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

x64::PageTable *VirtualAllocator::findPageTable(const x64::PageMapLevel2 *l2, uint16_t pdte) const {
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

void VirtualAllocator::mapRange4k(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
        map4k(i, vaddr, flags, type);
        vaddr.address += x64::kPageSize;
    }
}

void VirtualAllocator::mapRange2m(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type) {
    for (PhysicalAddress i = range.front; i < range.back; i += x64::kLargePageSize) {
        map2m(i, vaddr, flags, type);
        vaddr.address += x64::kLargePageSize;
    }
}

void VirtualAllocator::map4k(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, MemoryType type) {
    uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (vaddr.address >> 21) & 0b0001'1111'1111;
    uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

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

void VirtualAllocator::map2m(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, MemoryType type) {
    uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (vaddr.address >> 21) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
    x64::PageMapLevel3 *l3 = getPageMap3(l4, pml4e);
    x64::PageMapLevel2 *l2 = getPageMap2(l3, pdpte);

    x64::pde& t2 = l2->entries[pdte];
    t2.set2m(true);
    mPageManager->setMemoryType(t2, type);
    setEntryFlags(t2, flags, paddr.address);
}

void VirtualAllocator::mapRange(MemoryRange range, VirtualAddress vaddr, PageFlags flags, MemoryType type) {
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
            mapRange2m(body, vaddr + head.size(), flags, type);

            // finally map the trailing 4k pages
            MemoryRange tail = {back2m, range.back};
            mapRange4k(tail, vaddr + head.size() + body.size(), flags, type);
            return;
        }
    }

    // if we get to this point its not worth using 2m pages
    // so we just map the range with 4k pages
    mapRange4k(range, vaddr, flags, type);
}

void VirtualAllocator::unmap(void *ptr, size_t size) {
    VirtualAddress front = { sm::rounddown((uintptr_t)ptr, x64::kPageSize) };
    VirtualAddress back = { sm::roundup((uintptr_t)ptr + size, x64::kPageSize) };

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
    
    for (VirtualAddress i = front; i < back; i += x64::kPageSize) {
        uint16_t pml4e = (i.address >> 39) & 0b0001'1111'1111;
        uint16_t pdpte = (i.address >> 30) & 0b0001'1111'1111;
        uint16_t pdte = (i.address >> 21) & 0b0001'1111'1111;
        uint16_t pte = (i.address >> 12) & 0b0001'1111'1111;

        const x64::PageMapLevel3 *l3 = findPageMap3(l4, pml4e);
        if (!l3) continue;
        
        const x64::PageMapLevel2 *l2 = findPageMap2(l3, pdpte);
        if (!l2) continue;

        x64::PageTable *pt = findPageTable(l2, pdte);
        if (!pt) continue;

        x64::pte& t1 = pt->entries[pte];
        t1.setPresent(false);
        x64::invlpg(i.address);
    }
}

static void MapKernelPages(VirtualAllocator& memory, km::PhysicalAddress paddr, km::VirtualAddress vaddr) {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags) {
        km::PhysicalAddress front = km::PhysicalAddress {  (uintptr_t)begin - (uintptr_t)__kernel_start };
        km::PhysicalAddress back = km::PhysicalAddress { (uintptr_t)end - (uintptr_t)__kernel_start };

        memory.mapRange({ paddr + front.address, paddr + back.address }, vaddr + front.address, flags);
    };

    mapKernelRange(__text_start, __text_end, PageFlags::eCode);
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead);
    mapKernelRange(__data_start, __data_end, PageFlags::eData);
}

static void MapStage1Memory(VirtualAllocator& memory, const km::PageManager& pm, const SystemMemoryLayout& layout) {
    for (MemoryMapEntry range : layout.available) {
        memory.mapRange(range.range, km::VirtualAddress { range.range.front.address + pm.hhdmOffset() }, PageFlags::eData);
    }

    for (MemoryMapEntry range : layout.reclaimable) {
        memory.mapRange(range.range, km::VirtualAddress { range.range.front.address + pm.hhdmOffset() }, PageFlags::eData);
    }
}

void KmMapKernel(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout, km::PhysicalAddress paddr, km::VirtualAddress vaddr) {
    // first map the kernel pages
    MapKernelPages(vmm, paddr, vaddr);

    // then remap the rest of memory to the higher half
    MapStage1Memory(vmm, pm, layout);
}

void KmMigrateMemory(km::VirtualAllocator& vmm, km::PageManager& pm, const void *address, size_t size, km::MemoryType type) {
    km::PhysicalAddress base = km::PhysicalAddress { (uintptr_t)address - pm.hhdmOffset() };
    km::PhysicalAddress end = base + size;

    vmm.mapRange({ base, end }, km::VirtualAddress { base.address + pm.hhdmOffset() }, PageFlags::eData, type);
}

void KmReclaimBootMemory(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout) {
    // then apply the new page tables
    pm.setActiveMap(((uintptr_t)vmm.rootPageTable() - pm.hhdmOffset()));

    layout.reclaimBootMemory();

    KmDebugMessage("[INIT] Bootloader memory reclaimed.\n");
}
