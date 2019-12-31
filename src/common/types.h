#pragma once

#define SIZED_TYPE(name, type, size) using name = type; static_assert(sizeof(name) == size);

namespace bezos
{
    SIZED_TYPE(u8, unsigned char, 1);
    SIZED_TYPE(u16, unsigned short, 2);
    SIZED_TYPE(u32, unsigned int, 4);
    SIZED_TYPE(u64, unsigned long long, 8);

    SIZED_TYPE(i8, signed char, 1);
    SIZED_TYPE(i16, signed short, 2);
    SIZED_TYPE(i32, signed int, 4);
    SIZED_TYPE(i64, signed long long, 8);
}