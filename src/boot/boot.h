#pragma once

typedef unsigned long long u64;
typedef unsigned int u32;

typedef struct {
    u64 addr;
    u64 size;
    u32 type;
    u32 attrib;
} e820_entry_t;

typedef struct {
    e820_entry_t* memory;
    u32 parts;
} bootinfo_t;