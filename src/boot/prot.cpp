#include "types.h"

#define EXPORT [[gnu::section(".prot")]] extern "C"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((uint16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, colour) (letter | colour << 8)

#define VMA 0xFFFFFFFF80000000

#define ALIGN(addr) (addr + (-addr & (0x1000 - 1)))

extern "C" {
    uint32 KERN_ADDR;
    uint32 KERN_SIZE;
    uint32 KERN_LMA;
}

[[gnu::section(".prot")]]
static uint64 ptr;

[[gnu::section(".prot")]]
void alloc_init()
{
    // memory starts after the kernel aligned to page
    ptr = ALIGN(((uint64)&KERN_ADDR + (uint64)&KERN_SIZE) - (uint64)&KERN_LMA);
}

[[gnu::section(".prot")]]
uint64 alloc(uint32 size)
{
    uint64 out = ptr;
    ptr += size;

    for(int i = 0; i < size; i++)
        ((uint64*)out)[i] = 0;
    
    return out;
}

#define PT(val) ((uint64*)val)

#define ACCESS_PML(pml, idx, next) if(PT(pml)[idx] & 1) { next = PT(pml)[idx] & 0xFFFFFFFFFFFFF000; } else { next = (uint64)alloc(0x1000); PT(pml)[idx] = next | 0b111; }

EXPORT void paging()
{
    alloc_init();

    auto pml4 = alloc(0x1000);
    auto pml3 = alloc(0x1000);
    auto pml2 = alloc(0x1000);
    auto pml1 = alloc(0x1000);

    PT(pml4)[0] = pml3 | 0b111;
    PT(pml3)[0] = pml2 | 0b111;
    PT(pml2)[0] = pml1 | 0b111;

    // identity map the first mb
    for(int i = 0; i < 256; i++)
        PT(pml1)[i] = i * 0x1000;

    // map the kernel to the higher half
    {
        //auto addr = (uint64)&KERN_ADDR;
        //auto size = (uint64)&KERN_SIZE;
        //auto* kern = (uint64*)alloc(ALIGN(size));

        // copy the kernel to the new pages
        //for(int i = 0; i < size / 8; i++)
        //    kern[i] = ((uint64*)((uint64)&KERN_LMA))[i];

        //for(int i = 0 ; i < (size / 0x1000); i++)
        //    map_page(pml4, (uint64)kern + (i * 0x1000), addr + (i * 0x1000));
    }

    // load the page table into cr3 to use it
    asm("movl %%eax, %%cr3" :: "a"((uint64*)pml4));
}

EXPORT void clear()
{
    // grey on black colour 
    auto colour = VGA_COLOUR(7, 0);

    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = VGA_ENTRY(' ', colour);
}