#pragma once

#include "types.h"

namespace bezos::io
{
    // read in from port
    inline void out8(u16 port, u8 val)
    {
        asm volatile("outb %%al, %%dx" :: "a"(val), "d"(port));
    }

    inline void out16(u16 port, u16 val)
    {
        asm volatile("outw %%ax, %%dx" :: "a"(val), "d"(port));
    }

    inline void out32(u16 port, u16 val)
    {
        asm volatile("outl %%eax, %%dx" :: "a"(val), "d"(port));
    }


    // write out to port
    inline u8 in8(u16 port)
    {
        u8 ret;
        asm volatile("inb %%dx, %%al" : "=a"(ret) : "d"(port));
        return ret;
    }

    inline u16 in16(u16 port)
    {
        u16 ret;
        asm volatile("inw %%dx, %%ax" : "=a"(ret) : "d"(port));
        return ret;
    }

    inline u32 in32(u16 port)
    {
        u32 ret;
        asm volatile("inl %%dx, %%eax" : "=a"(ret) : "d"(port));
        return ret;
    }
}