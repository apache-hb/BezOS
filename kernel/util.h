#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define SECTION(name) __attribute__((section(name)))
#define PACKED __attribute__((__packed__))
