#ifndef IDT_H
#define IDT_H

#include "kernel/util/macros.h"
#include "kernel/util/types.h"

typedef struct PACKED {
    u16 off_low;
    u16 sel;
    u8 ist;
    u8 flags;
    u16 off_mid;
    u32 off_high;
    u32 reserved;
} idt_entry_t;

typedef struct PACKED {
    u16 limit;
    u64 base;
} idt_ptr_t;

idt_entry_t idt_entry(void *handle, u16 sel, u16 opts);

void lidt(idt_ptr_t idt);

#endif