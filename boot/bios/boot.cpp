#include <kernel.h>
#include <arch/idt.h>

#include <type_traits>

extern "C" {
    u64 KERNEL_BEGIN;
    u64 KERNEL_END;
    u64 PT_ADDR;
}

extern "C" u64 BASE_ADDR;

namespace {
    u64 base;

    template<typename T>
    T *bump(size_t num) {
        T *addr = (T*)base;
        base += sizeof(T) * num;
        return addr;
    }

    template<typename T, typename P>
    T add_table(P parent, u64 idx) {
        u64 *table = (u64*)parent;

        if (!(table[idx] & 1)) {
            auto entry = bump<u64>(512);

            for (int i = 0; i < 512; i++)
                entry[i] = 0;

            table[idx] = (u64)entry | 0b11;
            return (T)entry;
        } else {
            return (T)(table[idx] & ~(0x1000 - 1));
        }   
    }

    void map_page(vmm::pml4 pml4, u64 phys, u64 virt) {
        auto pml4_idx = (virt & (0x1FFULL << 39)) >> 39;
        auto pdpt_idx = (virt & (0x1FFULL << 30)) >> 30;
        auto pd_idx = (virt & (0x1FFULL << 21)) >> 21;
        auto pt_idx = (virt & (0x1FFULL << 12)) >> 12;

        auto pdpt = add_table<vmm::pdpt>(pml4, pml4_idx);
        auto pd = add_table<vmm::pd>(pdpt, pdpt_idx);
        auto pt = add_table<vmm::pt>(pd, pd_idx);

        u64 paddr = phys & ~(0x1000 - 1);
        pt[pt_idx] = paddr | 0b11;
    }
}

extern "C" void boot() {
    ((u16*)0xB8000)[0] = 'b' | 7 << 8;
    base = *(u64*)&BASE_ADDR;

    u16 count = *(u16*)(0x7FFFF - 2);
    auto *entries = (pmm::memory_map_entry*)(0x7FFFF - (2 + (sizeof(pmm::memory_map_entry) * count)));
    pmm::memory_map memory = { count, entries };

    auto pml4 = reinterpret_cast<vmm::pml4>(&PT_ADDR);

    auto kernel_pages = 0x100000 / 0x1000;

    for (int i = 0; i < kernel_pages; i++) {
        map_page(pml4, 0x100000 + (i * 0x1000), 0xFFFFFFFF80000000ULL + (i * 0x1000));
    }

    ((u16*)0xB8000)[0] = 'c' | 7 << 8;
    ((kmain_t*)0xFFFFFFFF80000000ULL)(memory, pml4);
}
