#pragma once

#include <stdint.h>
#include <stddef.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define SECTION(name) __attribute__((section(name)))
#define PACKED __attribute__((__packed__))
