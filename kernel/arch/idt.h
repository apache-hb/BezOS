#pragma once

#include <util.h>

typedef struct PACKED {
    u16 off_low;
    u16 sel;
    u8 ist;
    u8 attr;
    u16 off_mid;
    u32 off_high;
    u32 zero;
} idt_entry_t;

typedef struct PACKED {
    u16 limit;
    u64 base;
} idt_ptr_t;

typedef idt_entry_t idt_t[256];

#define LIDT(ptr) __asm__ volatile("lidt %0" :: "a"(ptr))
