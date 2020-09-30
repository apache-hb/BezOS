#include "kernel/kernel.h"
#include "kernel/util/macros.h"

extern u64 *PT_ADDR;
extern u64 *BASE_ADDR;
extern u64 VMA;

SECTION(".bootc")
void put(u8 i) {
    __asm__ volatile ("outb %0, %1" : : "a"(i), "Nd"(0xE9));
}

SECTION(".bootc")
void *bump(u64 size) {
    void *addr = BASE_ADDR;
    *BASE_ADDR += size;
    return addr;
}

SECTION(".bootc")
u64 *add_table(u64 *table, u64 idx, u64 flags) {
    if(!(table[idx] & 0b1)) {
        u64 *entry = bump(0x1000);
        
        // wipe the table as well
        for (int i = 0; i < 512; i++)
            entry[i] = 0;

        table[idx] = (u64)entry | flags;
    }

    return &table[idx];
}

SECTION(".bootc")
void map_huge_page(u64 paddr, u64 vaddr) {
    u64 pml4_idx = (vaddr & (0x1FFULL << 39)) >> 39;
    u64 pdpt_idx = (vaddr & (0x1FFULL << 30)) >> 30;
    u64 pd_idx = (vaddr & (0x1FFULL << 21)) >> 21;

    u64 *pdpt = add_table(PT_ADDR, pml4_idx, 0b11);
    u64 *pd = add_table(pdpt, pdpt_idx, 0b11);

    /* TODO: address is probably wrong */
    pd[pd_idx] = paddr | 0b10000011;
}

SECTION(".bootc")
void boot_main() {
    u16 count = *(u16*)(0x7FFFF - 2);
    memory_map_entry_t *entries = (memory_map_entry_t*)(0x7FFFF - (2 + (sizeof(memory_map_entry_t) * count)));
    memory_map_t memory = { count, entries };

    // map 1-15 mb to the higher half
    for (int i = 0; i < 7; i++) {
        map_huge_page(0x100000 + (i * 0x200000), (u64)&VMA + (i * 0x200000));
    }

    kmain(&memory, PT_ADDR);
}
