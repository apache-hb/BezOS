#include "kernel/kernel.h"
#include "kernel/util/macros.h"

extern u64 PT_ADDR[];
extern u64 *BASE_ADDR[];
extern u64 BSS_BEGIN[];
extern u64 BSS_END[];

extern u64 TEXT_BEGIN[];
extern u64 DATA_BEGIN[];
extern u64 RODATA_BEGIN[];
extern u64 BSS_BEGIN[];

extern u64 KERNEL_BEGIN[];
extern u64 KERNEL_PAGES[];

SECTION(".bootc")
char chars[] = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

#define PUT(i) { __asm__ volatile ("outb %0, %1" : : "a"((char)i), "Nd"(0xE9)); }
#define STR(s) { char str[] = s; int i = 0; while (str[i]) PUT(str[i++]) }
#define NUM(i, base) { char tstr[64] = { }; int idx = 0; \
    u64 num = i; u64 temp; \
    do { temp = num; num /= base; tstr[idx++] = (chars[35 + (temp - num * base)]); } while (num); \
    for (int id = idx; id--;) { PUT(tstr[id]) } \
    }

SECTION(".bootc")
void *bump(u64 size) {
    u64 addr = (u64)*BASE_ADDR;
    *BASE_ADDR += size;
    STR("allocated 0x") NUM((u64)addr, 16) PUT('\n')
    return (void*)addr;
}

SECTION(".bootc")
u64 *add_table(u64 *table, u64 idx, u64 flags) {
    u64 *entry = NULL;
    if (!(table[idx] & 1)) {
        entry = bump(0x1000);

        for (int i = 0; i < 512; i++)
            entry[i] = 0;

        table[idx] = (u64)entry | flags;
    } else {
        entry = (u64*)(table[idx] & ~0b11);
    }

    return entry;
}

SECTION(".bootc")
void map_page(u64 *pml4, u64 paddr, u64 vaddr) {
    STR("mapping 0x") NUM(paddr, 16) STR(" to 0x") NUM(vaddr, 16) PUT('\n')

    u64 pml4_idx = (vaddr & (0x1FFULL << 39)) >> 39;
    u64 pdpt_idx = (vaddr & (0x1FFULL << 30)) >> 30;
    u64 pd_idx = (vaddr & (0x1FFULL << 21)) >> 21;
    u64 pt_idx = (vaddr & (0x1FFULL << 12)) >> 12;

    u64 *pdpt = add_table(pml4, pml4_idx, 0b11);
    u64 *pd = add_table(pdpt, pdpt_idx, 0b11);
    u64 *pt = add_table(pd, pd_idx, 0b11);

    pt[pt_idx] = paddr | 0b11;

    STR("0x") NUM((u64)pml4, 16) STR(" pml4[") NUM((u64)pml4_idx, 10) STR("] = 0x") NUM(pml4[pml4_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pdpt, 16) STR(" pdpt[") NUM((u64)pdpt_idx, 10) STR("] = 0x") NUM(pdpt[pdpt_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pd, 16) STR(" pd[") NUM((u64)pd_idx, 10) STR("] = 0x") NUM(pd[pd_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pt, 16) STR(" pt[") NUM((u64)pt_idx, 10) STR("] = 0x") NUM(pt[pt_idx], 16) PUT('\n')
}

SECTION(".bootc")
void boot_main() {
    u16 count = *(u16*)(0x7FFFF - 2);
    memory_map_entry_t *entries = (memory_map_entry_t*)(0x7FFFF - (2 + (sizeof(memory_map_entry_t) * count)));
    memory_map_t memory = { count, entries };

    STR("boot = 0x") NUM((u64)boot_main, 16) PUT('\n')
    STR("pml4 = 0x") NUM((u64)PT_ADDR, 16) PUT('\n')
    STR("KERNEL_VADDR = 0x") NUM((u64)KERNEL_BEGIN, 16) PUT('\n')
    STR(".text = 0x") NUM((u64)TEXT_BEGIN, 16) PUT('\n')
    STR(".data = 0x") NUM((u64)DATA_BEGIN, 16) PUT('\n')
    STR(".rodata = 0x") NUM((u64)RODATA_BEGIN, 16) PUT('\n')
    STR(".bss = 0x") NUM((u64)BSS_BEGIN, 16) PUT('\n')
    STR("pages = ") NUM((u64)KERNEL_PAGES, 10) PUT('\n')
    STR("kmain = 0x") NUM((u64)kmain, 16) PUT('\n')
    STR("memory = (") NUM(memory.count, 10) STR(", 0x") NUM((u64)memory.entries, 16) STR(")\n") 

    for (int i = 0; i < (u64)KERNEL_PAGES; i++) {
        map_page(PT_ADDR, 0x100000 + (i * 0x1000), (u64)KERNEL_BEGIN + (i * 0x1000));
    }

    kmain(memory, PT_ADDR);
}
