#pragma once

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

static_assert(sizeof(uint8) == 1);
static_assert(sizeof(uint16) == 2);
static_assert(sizeof(uint32) == 4);
static_assert(sizeof(uint64) == 8);
