#ifndef KERNEL_H
#define KERNEL_H

#include "util/types.h"

typedef struct {
    u64 addr;
    u64 size;
    u32 type;
    u32 attrib;
} memory_map_entry_t;

typedef struct {
    size_t count;
    memory_map_entry_t *entries;
} memory_map_t;

void kmain(memory_map_entry_t *memory);

#endif 