#pragma once

#include "kernel/types.h"

namespace bezos::e820
{
    struct entry
    {
        u64 addr;
        u64 length;
        u32 type;
    } __attribute__((packed));
}