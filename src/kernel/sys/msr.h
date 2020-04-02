#pragma once

#include "common/types.h"

namespace msr
{
    inline uint64 read(uint64 m)
    {
        uint64 a;
        uint64 d;
        asm("rdmsr" : "=a"(a), "=d"(d) : "c"(m));

        return (d << 32) | a;
    }

    inline void write(uint64 m, uint64 data)
    {
        uint64 a = data & 0xFFFFFFFF;
        uint64 d = data >> 32;
        asm("wrmsr" :: "a"(a), "d"(d), "c"(m));
    }
}

namespace tsc
{
    uint64 read()
    {

    }
}