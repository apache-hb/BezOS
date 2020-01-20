#include "common/types.h"

#define VGA_BUFFER ((u16*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_COLOUR(fg, bg) (u16)(((u16)fg) | ((u16)bg << 4))
#define VGA_ENTRY(letter, colour) ((letter) | (colour << 8))

extern u32 KERNEL_END;

static u8 vga_column;
static u8 vga_row;

static u8 vga_colour(u8 fg, u8 bg) 
{
    return fg | bg << 4;
}

static u16 vga_entry(u16 c, u16 col)
{
    return c | col << 8;
}

static void vga_put(char c)
{
    if(c == '\n' || vga_column > VGA_WIDTH)
    {
        vga_column = 0;
        vga_row++;
        return;
    }

    if(vga_row > VGA_HEIGHT)
        vga_row = 0;

    VGA_BUFFER[vga_column + VGA_WIDTH * vga_row] = VGA_ENTRY(c, VGA_COLOUR(7, 0));
}

extern void vga_print(const char* str)
{
    while(*str)
        vga_put(*str++);
}

extern void vga_init()
{
    vga_column = 0;
    vga_row = 0;

    // clear vga buffer
    for(int i = 0; i < 80 * 25; i++)
        VGA_BUFFER[i] = vga_entry(' ', vga_colour(7, 0));
}

extern u64* init32()
{
    u64* page = (u64*)&KERNEL_END;

    // map the first 2 mb of memory
    // these pages will later be replaced by the main kernel
    // these are just here to get into long mode

    u64* pml4 = page;
    page += 0x1000;

    u64* pml3 = page;
    page += 0x1000;

    u64* pml2 = page;
    page += 0x1000;

    u64* pt = page;

    pml4[0] = (u64)pml3 | 3;
    pml3[0] = (u64)pml2 | 3;
    pml2[0] = (u64)pt | 3;

    // map the first 2mb directly to physical addresses
    for(int i = 0; i < 512; i++)
        pt[i] = (i * 0x1000) | 3;

    vga_print("aaaaaaaaaaaaaaaaaaaaaaaa");

    return pml4;
}
