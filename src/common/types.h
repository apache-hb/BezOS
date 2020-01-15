#pragma once

#define SIZED_TYPE(NAME, TYPE, SIZE) using NAME = TYPE; static_assert(sizeof(NAME) == SIZE);

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

    using byte = u8;
    using word = u16;
    using dword = u32;
    using qword = u64;
}