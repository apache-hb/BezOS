#pragma once

#include "types.h"

enum class E820Type : u32
{
    RAM = 1,
    Reserved = 2,
    ACPI = 3,
    Unusable = 4,
};

struct E820Entry
{
    void* addr;
    u64 len;
    u32 type;
    u32 attrib;
};

static_assert(sizeof(E820Entry) == 24);

// passed from the bootloader to kmain
struct BootData
{
    array<E820Entry> memory;
    void* page_table;
};

extern "C" void kmain(BootData* data);