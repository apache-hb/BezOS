#include "kernel/types.h"

#include "vga.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((u16*)0xB8000)

#define VGA_COLOUR(FG, BG) ((FG) | (BG) << 4)
#define VGA_ENTRY(C, COL) ((C) | (COL) << 8)

namespace bezos::vga
{
    u8 row;
    u8 column;
    u8 colour;

    void init()
    {
        row = 0;
        column = 0;
        colour = VGA_COLOUR(15, 0);
        for(u8 y = 0; y < VGA_HEIGHT; y++)
            for(u8 x = 0; x < VGA_WIDTH; x++)
                VGA_BUFFER[y * VGA_WIDTH + x] = VGA_ENTRY(' ', colour);
    }

    void put_at(char c, u8 x, u8 y)
    {
        VGA_BUFFER[y * VGA_WIDTH + x] = VGA_ENTRY(c, colour);
    }

    void put(char c)
    {
        put_at(c, column, row);
        if(++column == VGA_WIDTH || c == '\n')
        {
            column = 0;
            if(++row == VGA_HEIGHT)
                row = 0;
        }
    }

    void print(const char* str)
    {
        while(*str)
            put(*str++);
    }
}