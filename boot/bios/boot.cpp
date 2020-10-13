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

/*
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

SECTION(".boot")
char chars[] = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

#define PUT(i) { __asm__ volatile ("outb %0, %1" : : "a"((char)i), "Nd"(0xE9)); }
#define STR(s) { char str[] = s; int i = 0; while (str[i]) PUT(str[i++]) }
#define NUM(i, base) { char tstr[64] = { }; int idx = 0; \
    u64 num = i; u64 temp; \
    do { temp = num; num /= base; tstr[idx++] = (chars[35 + (temp - num * base)]); } while (num); \
    for (int id = idx; id--;) { PUT(tstr[id]) } }

SECTION(".boot")
static void *bump(u64 size) {
    u64 addr = (u64)*BASE_ADDR;
    *BASE_ADDR += size;
    return (void*)addr;
}

SECTION(".boot")
static u64 *add_table(u64 *table, u64 idx, u64 flags) {
    u64 *entry = NULL;
    if (!(table[idx] & 1)) {
        entry = bump(0x1000);

        for (int i = 0; i < 512; i++)
            entry[i] = 0;

        table[idx] = (u64)entry | flags;
    } else {
        entry = (u64*)(table[idx] & 0xfffffffffffff000);
    }

    return entry;
}

SECTION(".boot")
static void map_page(u64 *pml4, u64 paddr, u64 vaddr) {
    STR("mapping 0x") NUM(paddr, 16) STR(" to 0x") NUM(vaddr, 16) PUT('\n')

    u64 pml4_idx = (vaddr & (0x1FFULL << 39)) >> 39;
    u64 pdpt_idx = (vaddr & (0x1FFULL << 30)) >> 30;
    u64 pd_idx = (vaddr & (0x1FFULL << 21)) >> 21;
    u64 pt_idx = (vaddr & (0x1FFULL << 12)) >> 12;

    u64 *pdpt = add_table(pml4, pml4_idx, 0b11);
    u64 *pd = add_table(pdpt, pdpt_idx, 0b11);
    u64 *pt = add_table(pd, pd_idx, 0b11);

    u64 pa = paddr & ~(0x1000 - 1);
    pt[pt_idx] = pa | 0b11;

    INVLPG(pa);
    STR("0x") NUM((u64)pml4, 16) STR(" pml4[") NUM((u64)pml4_idx, 10) STR("] = 0x") NUM(pml4[pml4_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pdpt, 16) STR(" pdpt[") NUM((u64)pdpt_idx, 10) STR("] = 0x") NUM(pdpt[pdpt_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pd, 16) STR(" pd[") NUM((u64)pd_idx, 10) STR("] = 0x") NUM(pd[pd_idx], 16) PUT('\n')
    STR("0x") NUM((u64)pt, 16) STR(" pt[") NUM((u64)pt_idx, 10) STR("] = 0x") NUM(pt[pt_idx], 16) PUT('\n')
}

SECTION(".boot")
idt_entry_t idt[256];

SECTION(".boot")
__attribute__((interrupt))
void int_handler(void *frame) {
    __asm__ volatile("cli");
    STR("int\n")
    for (;;) { }
}

SECTION(".boot")
static idt_entry_t entry(void *func, u16 sel, u8 ist, u8 flags) {
    u64 addr = (u64)func;
    idt_entry_t entry = { 
        (u16)addr, sel, ist, flags, 
        (u16)(addr >> 16), (u32)(addr >> 32), 0
    };
    return entry;
}

SECTION(".boot")
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
    STR("idt = 0x") NUM((u64)idt, 16) PUT('\n')


    idt[0xE] = entry(int_handler, 0x8, 0, 0x8E);

    for (int i = 0; i < 256; i++) {
        idt[i] = entry(int_handler, 0x8, 0, 0x8E);
    }

    __asm__ volatile("nop");
    idt_ptr_t ptr = { (u16)(sizeof(idt_entry_t) * 256) - 1, (u64)idt };
    for (;;) { }
    __asm__ volatile (
        "lidt %0\n"
        "sti" 
        :: "m"(ptr)
    );

    for (int i = 0; i < (u64)KERNEL_PAGES; i++) {
        map_page(PT_ADDR, 0x100000 + (i * 0x1000), (u64)KERNEL_BEGIN + (i * 0x1000));
    }
    

    STR("kmain = 0x") NUM((u64)kmain, 16) PUT('\n')
    kmain(memory, PT_ADDR);
    for (;;) { }
}
*/