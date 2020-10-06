#pragma once

#include <util.h>

namespace x64 {
    struct PACKED idt_entry {
        u16 off_low;
        u16 sel;
        u8 ist;
        u8 attr;
        u16 off_mid;
        u32 off_high;
        u32 zero;
    };

    struct PACKED idt_ptr {
        u16 limit;
        u64 base;
    };

    using idt = idt_entry[256];
}

#define LIDT(ptr) __asm__ volatile("lidt %0" :: "a"(ptr))
