#include "vga.h"

#include "common/types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((uint16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, colour) (letter | colour << 8)

static int vga_row;
static int vga_column;

void vga_init()
{
    vga_row = 0;
    vga_column = 0;

    // vga will be cleared by the bootloader so no need to do it here
}

static void vga_put(char c)
{
    if(c == '\n')
    {
        vga_row++;
        vga_column = 0;
        return;
    }
    
    if(vga_column == VGA_WIDTH)
    {
        vga_column = 0;
        
        if(++vga_row == VGA_HEIGHT)
        {
            vga_row = 0;
        }
    }

    VGA_BUFFER[vga_row * VGA_WIDTH + vga_column] = VGA_ENTRY(c, VGA_COLOUR(7, 0));

    vga_column++;
}

void vga_print(const char* str)
{
    while(*str)
        vga_put(*str++);
}

void vga_puts(const char* str)
{
    vga_print(str);
    vga_put('\n');
}

void vga_puti(int val, int base)
{
    if(base == 16)
    {
        vga_print("0x");
    } 
    else if(base == 2)
    {
        vga_print("0b");
    }

    if(!val)
    {
        vga_put('0');
        return;
    }
    
    char buf[32] = {0};

    int i = 30;

    for(; val && i; --i, val /= base)
        buf[i] = "0123456789ABCDEF"[val % base];

    vga_print(&buf[i+1]);
    vga_put('\n');
}