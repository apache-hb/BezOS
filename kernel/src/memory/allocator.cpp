#include "memory/allocator.hpp"

#include "kernel.hpp"
#include "memory/paging.hpp"
#include "util/memory.hpp"

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

static size_t CountUsablePages(const SystemMemoryLayout& layout) noexcept {
    size_t count = 0;

    for (const MemoryRange& range : layout.available) {
        count += range.size();
    }

    for (const MemoryRange& range : layout.reclaimable) {
        count += range.size();
    }

    return count / x64::kPageSize;
}

static size_t GetMemoryBitmapSize(const SystemMemoryLayout& layout) noexcept {
    return CountUsablePages(layout) / CHAR_BIT;
}

SystemAllocator::SystemAllocator(const SystemMemoryLayout *layout) noexcept
    : mLayout(layout)
{
    // number of bytes required to store the bitmap
    size_t size = GetMemoryBitmapSize(*mLayout);

    // find first available memory range
    MemoryRange range = mLayout->available[0];

    KM_CHECK(range.size() >= size, "Not enough memory to store bitmap.");

    KmDebugMessage("[INIT] Physical memory bitmap size: ", km::bytes(size), ".\n");
}

/// page allocator

MemoryRange PageAllocator::currentRange() const noexcept {
    return mLayout->available[mCurrentRange];
}

void PageAllocator::setCurrentRange(int range) noexcept {
    KM_CHECK(range < mLayout->available.count(), "Out of memory.");

    mCurrentRange = range;
    mOffset = currentRange().front;
}

PageAllocator::PageAllocator(const SystemMemoryLayout *layout) noexcept
    : mLayout(layout)
{
    setCurrentRange(0);
}

PhysicalPointer<x64::page> PageAllocator::alloc4k() noexcept {
    if (currentRange().contains(mOffset + x64::kPageSize)) {
        x64::page *result = mOffset.as<x64::page>();
        mOffset += x64::kPageSize;
        return PhysicalPointer{result};
    }

    setCurrentRange(mCurrentRange + 1);

    return alloc4k();
}

// stage 1 allocator

struct Stage1Memory {
    const km::PageManager *pager;
    PageAllocator *allocator;
    x64::page *root;

    Stage1Memory(const km::PageManager *pm, PageAllocator *alloc) noexcept
        : pager(pm)
        , allocator(alloc)
        , root(alloc4k())
    { }

    void setEntryFlags(x64::Entry& entry, PageFlags flags, PhysicalAddress address) noexcept {
        entry.setPresent(true);
        pager->setAddress(entry, address.address);

        entry.setWriteable(bool(flags & PageFlags::eWrite));
        entry.setExecutable(bool(flags & PageFlags::eExecute));
    }

    x64::page *alloc4k() noexcept {
        PhysicalPointer<x64::page> result = allocator->alloc4k();
        uintptr_t offset = (uintptr_t)result.data + pager->hhdmOffset();
        x64::page *page = (x64::page*)offset;
        memset(page, 0, sizeof(x64::page));
        return page;
    }

    x64::page *rootPageTable() noexcept {
        return root;
    }

    void map3(PhysicalAddress paddr, VirtualAddress vaddr, x64::PageMapLevel3 *l3, PageFlags flags) noexcept {
        uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
        uint16_t pdte = (vaddr.address >> 21) & 0b0001'1111'1111;
        uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

        x64::PageMapLevel2 *l2;

        x64::pdpte& t3 = l3->entries[pdpte];
        if (!t3.present()) {
            l2 = std::bit_cast<x64::PageMapLevel2*>(alloc4k());
            setEntryFlags(t3, PageFlags::eAll, (uintptr_t)l2 - pager->hhdmOffset());
        } else {
            l2 = std::bit_cast<x64::PageMapLevel2*>(pager->address(t3) + pager->hhdmOffset());
        }

        x64::PageTable *pt;

        x64::pde& t2 = l2->entries[pdte];
        if (!t2.present()) {
            pt = std::bit_cast<x64::PageTable*>(alloc4k());
            setEntryFlags(t2, PageFlags::eAll, (uintptr_t)pt - pager->hhdmOffset());
        } else {
            pt = std::bit_cast<x64::PageTable*>(pager->address(t2) + pager->hhdmOffset());
        }

        x64::pte& t1 = pt->entries[pte];
        setEntryFlags(t1, flags, paddr.address);
    }

    void map(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags) noexcept {
        uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;

        x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)rootPageTable();
        x64::PageMapLevel3 *l3;

        x64::pml4e& t4 = l4->entries[pml4e];
        if (!t4.present()) {
            l3 = std::bit_cast<x64::PageMapLevel3*>(alloc4k());
            setEntryFlags(t4, PageFlags::eAll, (uintptr_t)l3 - pager->hhdmOffset());
        } else {
            l3 = std::bit_cast<x64::PageMapLevel3*>(pager->address(t4) + pager->hhdmOffset());
        }

        map3(paddr, vaddr, l3, flags);
    }
};

static void MapKernelPages(Stage1Memory& memory, limine_kernel_address_response address) noexcept {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags) {
        km::PhysicalAddress front = km::PhysicalAddress {  (uintptr_t)begin - (uintptr_t)__kernel_start };
        km::PhysicalAddress back = km::PhysicalAddress { (uintptr_t)end - (uintptr_t)__kernel_start };

        for (uintptr_t i = front.address; i < back.address; i += x64::kPageSize) {
            memory.map(km::PhysicalAddress { address.physical_base + i }, km::VirtualAddress { address.virtual_base + i }, flags);
        }
    };

    mapKernelRange(__text_start, __text_end, PageFlags::eCode);
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead);
    mapKernelRange(__data_start, __data_end, PageFlags::eData);
}

[[maybe_unused]]
static void MapStage1Memory(Stage1Memory& memory, const km::PageManager& pm, const SystemMemoryLayout& layout) noexcept {
    for (MemoryRange range : layout.available) {
        for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
            memory.map(i, km::VirtualAddress { i.address + pm.hhdmOffset() }, PageFlags::eData);
        }
    }

    for (MemoryRange range : layout.reclaimable) {
        for (PhysicalAddress i = range.front; i < range.back; i += x64::kPageSize) {
            memory.map(i, km::VirtualAddress { i.address + pm.hhdmOffset() }, PageFlags::eData);
        }
    }
}

/// MapKernelStage1 is responsible for remapping the kernel and higher half mappings.
static void MapKernelStage1(
    Stage1Memory& memory,
    const km::PageManager& pm,
    const km::SystemMemoryLayout& layout,
    limine_kernel_address_response address
) noexcept {
    // first map the kernel pages
    MapKernelPages(memory, address);

    // then remap the rest of memory to the higher half
    MapStage1Memory(memory, pm, layout);

    // then apply the new page tables
    pm.setActiveMap(((uintptr_t)memory.rootPageTable() - pm.hhdmOffset()));

    KmDebugMessage("[INIT] Stage1 page tables applied.\n");
}

void KmMapKernel(
    const km::PageManager& pm,
    km::SystemAllocator&,
    const km::SystemMemoryLayout& layout,
    limine_kernel_address_response address
) noexcept {
    PageAllocator allocator{&layout};
    Stage1Memory memory{&pm, &allocator};
    MapKernelStage1(memory, pm, layout, address);
}
