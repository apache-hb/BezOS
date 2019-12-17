#include "vga.h"

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 20
#define VGA_OFFSET 0xB8000

namespace bezos::vga
{
    u16 vga_colour(u16 fg, u16 bg)
    {
        return fg | bg << 4;
    }

    u16 vga_entry(u16 c, u16 col)
    {
        return c | col << 8;
    }

    u16* output;
    u8 row;
    u8 column;
    u8 colour;
    
    void init()
    {
        row = 0;
        column = 0;
        colour = vga_colour(8, 0);
        output = (u16*)VGA_OFFSET;

        for(u8 y = 0; y < VGA_WIDTH; y++)
        {
            for(u8 x = 0; x < VGA_HEIGHT; x++)
            {
                output[y * VGA_WIDTH + x] = vga_entry('a', colour);
            }
        }
    }

    void putchar(u16 c) 
    {
        
    }

    void write(const char* str)
    {
        while(*str) 
        {
            if(*str == '\n')
            {
                row = 0;
                column++;
                str++;
                continue;
            }
            putchar(*str);
            str++;
        }
    }
}