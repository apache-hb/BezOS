#ifndef KERNEL_MM_PAGING_H
#define KERNEL_MM_PAGING_H 1

#include "common/types.h"

typedef struct {
    u64 len;
    u64 addr;
    u32 type;
    u32 attrib;
} e820_entry;

STATIC_ASSERT(sizeof(e820_entry) == 24, "e820_entry should be 24 bytes wide");

#define PAGE_ALIGN(addr) ((addr + 0x1000 - 1) & -0x1000)

#endif // KERNEL_MM_PAGING_H
