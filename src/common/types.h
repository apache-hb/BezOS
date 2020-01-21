#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H 1

#include "macros.h"

#define SIZED_TYPE(name, type, size) typedef type name; STATIC_ASSERT(sizeof(name) == size, #name " must be " #size " bytes wide");

SIZED_TYPE(u8, unsigned char, 1)
SIZED_TYPE(u16, unsigned short, 2)
SIZED_TYPE(u32, unsigned int, 4)
SIZED_TYPE(u64, unsigned long long, 8)

SIZED_TYPE(i8, signed char, 1)
SIZED_TYPE(i16, signed short, 2)
SIZED_TYPE(i32, signed int, 4)
SIZED_TYPE(i64, signed long long, 8)

typedef u8 byte;
typedef u16 word;
typedef u32 dword;
typedef u64 qword;

#endif //COMMON_TYPES_H
