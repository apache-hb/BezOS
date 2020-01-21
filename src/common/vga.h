#pragma once

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((bezos::u16*)0xB8000)

#define VGA_COLOUR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(letter, col) (letter | col << 8)

// a bit of a hack to prevent duplicate symbols
#ifndef PREFIX
#   define PREFIX(func) func
#endif

namespace bezos::vga
{
    extern u8 column;
    extern u8 row;
    extern u8 colour;

    void PREFIX(put)(char c)
    {
        // handle newlines
        if(c == '\n' || column > VGA_WIDTH)
        {
            column = 0;
            row++;
            return;
        }

        if(row > VGA_HEIGHT)
        {
            row = 0;
        }

        // write to vga
        VGA_BUFFER[column + VGA_WIDTH * row] = VGA_ENTRY(c, colour);

        column++;
    }

    void PREFIX(print)(const char* msg)
    {
        while(*msg)
            PREFIX(put)(*msg++);
    }

    void PREFIX(print)(int val, int base = 16)
    {
        if(base == 16)
            PREFIX(print)("0x");

        char buf[32] = {};

        int i = 30;

        for(; val && i; --i, val /= base)
            buf[i] = "0123456789ABCDEF"[val % base];

        PREFIX(print)(&buf[i+1]);
    }
}