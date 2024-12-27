#include "memory/allocator.hpp"

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

MemoryRange PageAllocator::currentRange() const {
    return mLayout->available[mCurrentRange];
}

void PageAllocator::setCurrentRange(int range) {
    KM_CHECK(range < mLayout->available.count(), "Out of memory.");

    mCurrentRange = range;
    mOffset = currentRange().front;
}

PageAllocator::PageAllocator(const SystemMemoryLayout *layout)
    : mLayout(layout)
{
    setCurrentRange(0);
}

PhysicalPointer<x64::page> PageAllocator::alloc4k() {
    if (currentRange().contains(mOffset + x64::kPageSize)) {
        uintptr_t result = mOffset.address;
        mOffset += x64::kPageSize;
        return PhysicalPointer{(x64::page*)result};
    }

    setCurrentRange(mCurrentRange + 1);

    return alloc4k();
}

x64::page *VirtualAllocator::alloc4k() {
    PhysicalPointer<x64::page> result = mPageAllocator->alloc4k();
    uintptr_t offset = (uintptr_t)result.data + mPageManager->hhdmOffset();
    x64::page *page = (x64::page*)offset;
    memset(page, 0, sizeof(x64::page));
    return page;
}

void VirtualAllocator::setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) {
    entry.setPresent(true);
    mPageManager->setAddress(entry, address.address);

    entry.setWriteable(bool(flags & PageFlags::eWrite));
    entry.setExecutable(bool(flags & PageFlags::eExecute));
}

VirtualAllocator::VirtualAllocator(const km::PageManager *pm, PageAllocator *alloc)
    : mPageManager(pm)
    , mPageAllocator(alloc)
    , mRootPageTable(alloc4k())
{ }

void VirtualAllocator::map4k(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags) {
    uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;
    uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
    uint16_t pdte = (vaddr.address >> 21) & 0b0001'1111'1111;
    uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

    x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
    x64::PageMapLevel3 *l3;
    x64::PageMapLevel2 *l2;
    x64::PageTable *pt;

    x64::pml4e& t4 = l4->entries[pml4e];
    if (!t4.present()) {
        l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
        setEntryFlags(t4, PageFlags::eAll, (uintptr_t)l3 - mPageManager->hhdmOffset());
    } else {
        l3 = std::bit_cast<x64::PageMapLevel3*>(mPageManager->address(t4) + mPageManager->hhdmOffset());
    }

    x64::pdpte& t3 = l3->entries[pdpte];
    if (!t3.present()) {
        l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
        setEntryFlags(t3, PageFlags::eAll, (uintptr_t)l2 - mPageManager->hhdmOffset());
    } else {
        l2 = std::bit_cast<x64::PageMapLevel2*>(mPageManager->address(t3) + mPageManager->hhdmOffset());
    }

    x64::pde& t2 = l2->entries[pdte];
    if (!t2.present()) {
        pt = std::bit_cast<x64::PageTable*>(alloc4k());
        setEntryFlags(t2, PageFlags::eAll, (uintptr_t)pt - mPageManager->hhdmOffset());
    } else {
        pt = std::bit_cast<x64::PageTable*>(mPageManager->address(t2) + mPageManager->hhdmOffset());
    }

    x64::pte& t1 = pt->entries[pte];
    setEntryFlags(t1, flags, paddr.address);
}

static void MapKernelPages(VirtualAllocator& memory, limine_kernel_address_response address) {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags) {
        km::PhysicalAddress front = km::PhysicalAddress {  (uintptr_t)begin - (uintptr_t)__kernel_start };
        km::PhysicalAddress back = km::PhysicalAddress { (uintptr_t)end - (uintptr_t)__kernel_start };

        for (uintptr_t i = front.address; i < back.address; i += x64::kPageSize) {
            memory.map4k(km::PhysicalAddress { address.physical_base + i }, km::VirtualAddress { address.virtual_base + i }, flags);
        }
    };

    mapKernelRange(__text_start, __text_end, PageFlags::eCode);
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead);
    mapKernelRange(__data_start, __data_end, PageFlags::eData);
}

static void MapStage1Memory(VirtualAllocator& memory, const km::PageManager& pm, const SystemMemoryLayout& layout) {
    for (MemoryRange range : layout.available) {
        for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
            memory.map4k(i, km::VirtualAddress { i.address + pm.hhdmOffset() }, PageFlags::eData);
        }
    }

    for (MemoryRange range : layout.reclaimable) {
        for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
            memory.map4k(i, km::VirtualAddress { i.address + pm.hhdmOffset() }, PageFlags::eData);
        }
    }
}

void KmMapKernel(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout, limine_kernel_address_response address) {
    PageAllocator allocator{&layout};

    // first map the kernel pages
    MapKernelPages(vmm, address);

    // then remap the rest of memory to the higher half
    MapStage1Memory(vmm, pm, layout);
}

void KmMigrateMemory(km::VirtualAllocator& vmm, km::PageManager& pm, const void *address, size_t size) {
    km::PhysicalAddress base = km::PhysicalAddress { (uintptr_t)address - pm.hhdmOffset() };
    km::PhysicalAddress end = base + size;

    for (km::PhysicalAddress i = base; i < end; i += x64::kPageSize) {
        vmm.map4k(i, km::VirtualAddress { i.address + pm.hhdmOffset() }, PageFlags::eData);
    }
}

void KmReclaimBootMemory(const km::PageManager& pm, km::VirtualAllocator& vmm, km::SystemMemoryLayout& layout) {
    // then apply the new page tables
    pm.setActiveMap(((uintptr_t)vmm.rootPageTable() - pm.hhdmOffset()));

    layout.reclaimBootMemory();

    KmDebugMessage("[INIT] Bootloader memory reclaimed.\n");
}
