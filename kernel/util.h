#pragma once

#include <stdint.h>
#include <stddef.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define SECTION(name) __attribute__((section(name)))
#define PACKED __attribute__((__packed__))
#define MS_ABI __attribute__((ms_abi))

#define ASM(...) __asm__ volatile (__VA_ARGS__)

extern "C" void __cxa_pure_virtual();

void operator delete(void*);