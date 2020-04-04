#include "types.h"

#define EXPORT [[gnu::section(".prot")]] extern "C"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((uint16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, colour) (letter | colour << 8)

#define VMA 0xFFFFFFFF80000000

extern "C" {
    uint32 KERN_ADDR;
    uint32 KERN_SIZE;
}

[[gnu::always_inline]]
[[gnu::section(".prot")]]
void map_page(uint64**** pml4, uint64 paddr, uint64 vaddr)
{
    auto pml4_idx = (vaddr & ((uint64)0x1FF << 39)) >> 39;
    auto pml3_idx = (vaddr & ((uint64)0x1FF << 30)) >> 30;
    auto pml2_idx = (vaddr & ((uint64)0x1FF << 21)) >> 21;
    auto pml1_idx = (vaddr & ((uint64)0x1FF << 12)) >> 12;

    auto* pml3 = pml4[pml4_idx];
    auto* pml2 = pml3[pml3_idx];
    auto* pml1 = pml2[pml2_idx];

    pml1[pml1_idx] = paddr | 3;
}

EXPORT void paging()
{
    uint64**** pml = reinterpret_cast<uint64****>((uint32)&KERN_ADDR + (uint32)&KERN_SIZE);
    for(int i = 0; i < 512; i++)
        pml[i] = 0;
    //for(int i = 0; i < KERN_SIZE / 0x1000; i++)
    //    map_page(pml, (uint64)&KERN_ADDR + (i * 0x1000) - VMA, (uint64)&KERN_ADDR + (i * 0x1000));


    for(int i = 0; i < 512; i++)
        map_page(pml, i * 0x1000, i * 0x1000);

    asm("movl %%eax, %%cr3" :: "a"(pml));
}

EXPORT void clear()
{
    // grey on black colour 
    auto colour = VGA_COLOUR(7, 0);

    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = VGA_ENTRY(' ', colour);
}