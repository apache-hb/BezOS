#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

using i8 = unsigned char;
using i16 = unsigned short;
using i32 = unsigned int;
using i64 = unsigned long long;

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

using byte = u8;
using word = u16;
using dword = u32;
using qword = u64;

template<typename T>
struct array
{
    u32 len;
    T* data;

    T* begin() const { return data; }
    T* end() const { return data + len; }
};