#include "common/types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((u16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, col) (letter | col << 8)

typedef u8 vga_colour_t;

static u8 vga_row;
static u8 vga_column;

static void vga_put(char c)
{
    if(c == '\n')
    {
        vga_column = 0;
        vga_row++;
        return;
    }

    if(vga_row > VGA_HEIGHT)
        vga_row = 0;

    vga_colour_t colour = VGA_COLOUR(7, 0);

    VGA_BUFFER[vga_column + VGA_WIDTH * vga_row] = VGA_ENTRY(c, colour);

    vga_column++;
}

extern void vga_print(const char* str)
{
    while(*str)
        vga_put(*str++);
}

extern void vga_init(void)
{
    vga_row = 0;
    vga_column = 0;

    vga_colour_t colour = VGA_COLOUR(7, 0);

    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = VGA_ENTRY(' ', colour);
}

#define PAGE_ALIGN(val) ((val + 0x1000 -1) & -0x1000)

extern int LOW_MEMORY;

extern u64* init32(void)
{
    u64* page = (u64*)PAGE_ALIGN((u64)&LOW_MEMORY);
    (void)page;

    u64* pml4 = page | 3;
    page += 0x1000;

    u64* pml3 = page | 3;
    page += 0x1000;

    u64* pml2 = page | 3;
    page += 0x1000;

    u64* pt = page | 3;
    for(int i = 0; i < 512; i++)
        pt[i] = (i * 0x1000) | 3;

    return pml4;
}
