#pragma once

#include "types.h"

namespace bezos
{
    inline void out8(u16 port, u8 val)
    {
        asm volatile( "outb %0, %1" :: "a"(val), "Nd"(port));
    }

    inline u8 in8(u16 port)
    {
        u8 ret;
        asm volatile( "inb %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }
}