#include "memory/allocator.hpp"

#include "kernel.hpp"
#include "util/memory.hpp"

#include <bit>
#include <limits.h>
#include <string.h>

using namespace km;

extern "C" {
    extern char __text_start[]; // always aligned to 0x1000
    extern char __text_end[];

    extern char __rodata_start[]; // always aligned to 0x1000
    extern char __rodata_end[];

    extern char __data_start[]; // always aligned to 0x1000
    extern char __data_end[];

    extern char __bss_start[]; // always aligned to 0x1000
    extern char __bss_end[];

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

SystemAllocator::SystemAllocator(const SystemMemoryLayout *layout) noexcept
    : mLayout(layout)
{
    // number of pages required to store the bitmap
    size_t pages = CountUsablePages(*mLayout);

    // number of bytes required to store the bitmap
    size_t size = pages / CHAR_BIT;

    // find first available memory range
    MemoryRange range = mLayout->available[0];

    KM_CHECK(range.size() >= size, "Not enough memory to store bitmap.");

    KmDebugMessage("[INIT] Physical memory bitmap size: ", km::bytes(size), ".\n");

#if 0
    mBitMap = range.front.as<uint8_t>();
    mCount = size;
    memset(mBitMap, 0, size);

    KmDebugMessage("[INIT] Physical memory bitmap at ", Hex(range.front.address).pad(8, '0'), " size ", size, " pages.\n");

    const void *map = pm.activeMap();

    auto markRangeUsed = [&](const void *start, const void *end, stdx::StringView name) {
        km::PhysicalAddress front = KmVirtualToPhysical(pm, map, VirtualAddress((uintptr_t)start));
        km::PhysicalAddress back = KmVirtualToPhysical(pm, map, VirtualAddress((uintptr_t)end));
        KmDebugMessage("[INIT] Marking segment ", name, " as used: ", Hex(front.address).pad(8, '0'), " - ", Hex(back.address).pad(8, '0'), "\n");
        markUsed(MemoryRange(front, back));
    };

    // mark the bitmap as used
    markUsed(MemoryRange(range.front, range.front.address + size));

    // mark kernel sections as used
    markRangeUsed(__text_start, __text_end, ".text");
    markRangeUsed(__rodata_start, __rodata_end, ".rodata");
    markRangeUsed(__data_start, __data_end, ".data");
    markRangeUsed(__bss_start, __bss_end, ".bss");
#endif
}

static km::PhysicalAddress VirtualToPhysical3(const x64::PageManager& pm, const x64::PageMapLevel3 *pml3, km::VirtualAddress vaddr) noexcept {
    uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
    uint16_t pde = (vaddr.address >> 21) & 0b0001'1111'1111;
    uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

    KmDebugMessage("[INIT] pdpte: ", pdpte, " pde: ", pde, " pte: ", pte, "\n");
    KmDebugMessage("[INIT] pml3: ", Hex((uintptr_t)pml3).pad(8, '0'), "\n");

    const x64::pdpte *l3 = &pml3->entries[pdpte];
    KmDebugMessage("[INIT] l3: ", Hex((uintptr_t)l3).pad(8, '0'), "\n");
    if (!l3->present())
        return nullptr;

    const x64::PageMapLevel2 *pml2 = reinterpret_cast<const x64::PageMapLevel2*>(pm.address(*l3));
    const x64::pde *l2 = &pml2->entries[pde];
    KmDebugMessage("[INIT] pml2: ", Hex((uintptr_t)pml2).pad(8, '0'), ", l2: ", Hex((uintptr_t)l2).pad(8, '0'), "\n");
    if (!l2->present())
        return nullptr;

    const x64::PageTable *pml1 = reinterpret_cast<const x64::PageTable*>(pm.address(*l2));
    const x64::pte *l1 = &pml1->entries[pte];
    KmDebugMessage("[INIT] pml1: ", Hex((uintptr_t)pml1).pad(8, '0'), ", l1: ", Hex((uintptr_t)l1).pad(8, '0'), "\n");
    if (!l1->present())
        return nullptr;

    return km::PhysicalAddress { pm.address(*l1) };
}

km::PhysicalAddress KmVirtualToPhysical(const x64::PageManager& pm, const void *pagemap, km::VirtualAddress vaddr) noexcept {
    if (pm.has4LevelPaging()) {
        uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;

        KmDebugMessage("[INIT] pml4e: ", pml4e, "\n");

        const x64::PageMapLevel4 *l4 = reinterpret_cast<const x64::PageMapLevel4*>(pagemap);

        KmDebugMessage("[INIT] l4: ", Hex((uintptr_t)l4).pad(8, '0'), "\n");

        const x64::pml4e *t4 = &l4->entries[pml4e];
        KmDebugMessage("[INIT] t4: ", Hex((uintptr_t)t4).pad(8, '0'), "\n");
        if (!t4->present())
            return nullptr;

        return VirtualToPhysical3(pm, (const x64::PageMapLevel3*)pm.address(*t4), vaddr);
    }

    return VirtualToPhysical3(pm, (const x64::PageMapLevel3*)pagemap, vaddr);
}

static void SetEntryFlags(x64::Entry& entry, PageFlags flags, const x64::PageManager& pm, PhysicalAddress address) noexcept {
    entry.setPresent(true);
    pm.setAddress(entry, address.address);

    if (bool(flags & PageFlags::eWrite))
        entry.setWriteable(true);

    if (bool(flags & PageFlags::eExecute))
        entry.setExecutable(true);
}

struct Stage1Memory {
    x64::page pages[512];
    int used = 0;

    Stage1Memory() noexcept {
        x64::page& pm = alloc4k(); // allocate the top level page table
        memset(&pm, 0, sizeof(x64::page));
    }

    x64::page& alloc4k() noexcept {
        KM_CHECK(used < 512, "Out of memory.");
        x64::page& result = pages[used++];
        memset(&result, 0, sizeof(x64::page));
        return result;
    }

    x64::page *topLevelPage() noexcept {
        return &pages[0];
    }

    void map3(PhysicalAddress paddr, VirtualAddress vaddr, x64::PageMapLevel3 *l3, PageFlags flags, const x64::PageManager& pm) noexcept {
        uint16_t pdpte = (vaddr.address >> 30) & 0b0001'1111'1111;
        uint16_t pde = (vaddr.address >> 21) & 0b0001'1111'1111;
        uint16_t pte = (vaddr.address >> 12) & 0b0001'1111'1111;

        KmDebugMessage("[INIT] pdpte: ", pdpte, " pde: ", pde, " pte: ", pte, "\n");

        x64::PageMapLevel2 *l2;

        x64::pdpte& t3 = l3->entries[pdpte];
        if (!t3.present()) {
            l2 = std::bit_cast<x64::PageMapLevel2*>(&alloc4k());
            SetEntryFlags(t3, flags, pm, l2);
        } else {
            l2 = std::bit_cast<x64::PageMapLevel2*>(pm.address(t3));
        }

        KmDebugMessage("[INIT] l2: ", Hex((uintptr_t)l2).pad(8, '0'), "\n");

        x64::PageTable *pt;

        x64::pde& t2 = l2->entries[pde];
        if (!t2.present()) {
            pt = std::bit_cast<x64::PageTable*>(&alloc4k());
            SetEntryFlags(t2, flags, pm, pt);
        } else {
            pt = std::bit_cast<x64::PageTable*>(pm.address(t2));
        }

        x64::pte& t1 = pt->entries[pte];
        SetEntryFlags(t1, flags, pm, (const void*)paddr.address);
    }

    void map4(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, const x64::PageManager& pm) noexcept {
        uint16_t pml4e = (vaddr.address >> 39) & 0b0001'1111'1111;

        KmDebugMessage("[INIT] pml4e: ", pml4e, "\n");

        x64::PageMapLevel4 *l4 = (x64::PageMapLevel4*)topLevelPage();
        x64::PageMapLevel3 *l3;

        x64::pml4e& t4 = l4->entries[pml4e];
        if (!t4.present()) {
            l3 = std::bit_cast<x64::PageMapLevel3*>(&alloc4k());
            SetEntryFlags(t4, flags, pm, l3);
        } else {
            l3 = std::bit_cast<x64::PageMapLevel3*>(pm.address(t4));
        }

        map3(paddr, vaddr, l3, flags, pm);
    }

    void map(PhysicalAddress paddr, VirtualAddress vaddr, PageFlags flags, const x64::PageManager& pm) noexcept {
        if (pm.has4LevelPaging()) {
            map4(paddr, vaddr, flags, pm);
        } else {
            map3(paddr, vaddr, (x64::PageMapLevel3*)topLevelPage(), flags, pm);
        }
    }
};

// For stage1 we map the entire kernel as read/write/execute.
// If I wanted to map with permissions I would need to walk the existing page tables to find the correct offsets to map.
// As far as I know, there is no way to walk the bootloader page tables in stage1.
// As to walk the bootloader page tables, we need to know the virtual address of the page tables.
static void MapKernelPages(Stage1Memory& memory, const x64::PageManager& pm, limine_kernel_address_response address) noexcept {
    size_t kernelSize = (uintptr_t)__kernel_end - (uintptr_t)__kernel_start;
    size_t kernelPages = km::pages(kernelSize);

    KmDebugMessage("[INIT] Mapping kernel at ", Hex((uintptr_t)__kernel_start).pad(8, '0'), " - ", Hex((uintptr_t)__kernel_end).pad(8, '0'), " (", kernelPages, " pages).\n");

    for (uintptr_t i = 0; i < kernelPages; i++) {
        km::PhysicalAddress paddr = address.physical_base + (i * x64::kPageSize);
        km::VirtualAddress vaddr = { address.virtual_base + (i * x64::kPageSize) };
        memory.map(paddr, vaddr, PageFlags::eAll, pm);
    }

    KmDebugMessage("[INIT] Kernel mapped.\n");

#if 0
    auto mapKernelRange = [&](const void *begin, const void *end, stdx::StringView segment, PageFlags flags) {
        km::PhysicalAddress front = KmVirtualToPhysical(pm, map, VirtualAddress((uintptr_t)begin));
        km::PhysicalAddress back = KmVirtualToPhysical(pm, map, VirtualAddress((uintptr_t)end));
        KmDebugMessage("[INIT] Mapping segment '", segment, "' at ", Hex(front.address).pad(8, '0'), " - ", Hex(back.address).pad(8, '0'), ".\n");

        for (uintptr_t i = front.address; i < back.address; i += x64::kPageSize) {
            memory.map(km::PhysicalAddress { i }, km::VirtualAddress { i }, flags, pm);
        }
    };

    mapKernelRange(__text_start, __text_end, ".text", PageFlags::eCode);
    mapKernelRange(__rodata_start, __rodata_end, ".rodata", PageFlags::eRead);
    mapKernelRange(__data_start, __data_end, ".data", PageFlags::eData);
    mapKernelRange(__bss_start, __bss_end, ".bss", PageFlags::eData);
#endif
}

struct PageAllocator {
    uint8_t *bitmap;
    size_t words;
};

static void MapStage1Memory(Stage1Memory& memory, const x64::PageManager& pm, const SystemMemoryLayout& layout) noexcept {
    size_t pages = km::roundpow2(CountUsablePages(layout), size_t(CHAR_BIT));
    size_t words = pages / CHAR_BIT;

    MemoryRange range = layout.available[0];

    KmDebugMessage("[INIT] Stage1 memory bitmap of size ", km::bytes(words), " at ", Hex(range.front.address).pad(8, '0'), ".\n");

    KM_CHECK(range.size() >= words, "Not enough memory in first free area to store bitmap.");

    // Identity map the bitmap area.
    for (size_t i = 0; i < words; i++) {
        uintptr_t addr = range.front.address + i * CHAR_BIT;
        memory.map(km::PhysicalAddress { addr }, km::VirtualAddress { addr }, PageFlags::eData, pm);
    }
}

/// MapKernelStage1 is responsible for remapping the kernel and identity mapping enough usable memory
/// to allocate the full memory bitmap.
static void MapKernelStage1(const x64::PageManager& pm, const km::SystemMemoryLayout& layout, limine_kernel_address_response address) noexcept {
    Stage1Memory memory;

    KmDebugMessage("[INIT] Stage1 memory map.\n");

    // first map the kernel pages
    MapKernelPages(memory, pm, address);

    // then identity map enough memory to store the bitmap
    MapStage1Memory(memory, pm, layout);
}

void KmMapKernel(const x64::PageManager& pm, km::SystemAllocator&, const km::SystemMemoryLayout& layout, limine_kernel_address_response address) noexcept {
    MapKernelStage1(pm, layout, address);
}
