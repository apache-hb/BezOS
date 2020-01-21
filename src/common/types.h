#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H 1

#include "macros.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);

STATIC_ASSERT(sizeof(i8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);

typedef u8 byte;
typedef u16 word;
typedef u32 dword;
typedef u64 qword;

#if 0
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
#endif

#endif // COMMON_TYPES_H
