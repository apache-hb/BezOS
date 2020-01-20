#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H 1

#include "assert.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

STATIC_ASSERT(sizeof(u8) == 1, "u8 should be 1 byte wide");
STATIC_ASSERT(sizeof(u16) == 2, "u16 should be 2 bytes wide");
STATIC_ASSERT(sizeof(u32) == 4, "u32 should be 4 bytes wide");
STATIC_ASSERT(sizeof(u64) == 8, "u64 should be 8 bytes wide");

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

STATIC_ASSERT(sizeof(s8) == 1, "s8 should be 1 byte wide");
STATIC_ASSERT(sizeof(s16) == 2, "s16 should be 2 bytes wide");
STATIC_ASSERT(sizeof(s32) == 4, "s32 should be 4 bytes wide");
STATIC_ASSERT(sizeof(s64) == 8, "s64 should be 8 bytes wide");

#endif // COMMON_TYPES_H
