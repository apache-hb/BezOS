#pragma once

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((bezos::u16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, col) (letter | col << 8)

namespace bezos::vga
{
    u8 column;
    u8 row;
    u8 colour;

    void put(char c)
    {
        // handle newlines
        if(c == '\n' || column > VGA_WIDTH)
        {
            column = 0;
            row++;
        }

        if(row > VGA_HEIGHT)
        {
            row = 0;
        }

        // write to vga
        VGA_BUFFER[column + VGA_WIDTH * row] = VGA_ENTRY(c, colour);

        column++;
    }

    void init()
    {
        column = 0;
        row = 0;
        colour = VGA_COLOUR(7, 0);

        // TODO: clear vga buffer
    }

    void print(const char* msg)
    {
        while(*msg)
            put(*msg++);
    }

    void print(int val, int base = 16)
    {
        if(base == 16)
            print("0x");

        do
        {
            put("0123456789ABCDEF"[val % base]);
            val /= base;
        }
        while(val != 0);
    }
}