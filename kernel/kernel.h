#pragma once

#include <util.h>
#include <mm/mm.h>

typedef struct PACKED {
    u64 addr;
    u64 size;
    u32 type;
    u32 attrib;
} memory_map_entry_t;

typedef struct {
    size_t size;
    memory_map_entry_t *entries;
} memory_map_t;

void kmain(memory_map_t memory, pml4_t pml4);