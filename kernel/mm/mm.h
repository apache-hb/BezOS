#pragma once

#include <util.h>

namespace mm {
    using pte = u64;
    using pt = pte[512];
    using pd = pt[512];
    using pdpt = pd[512];
    using pml4 = pdpt[512];

    struct PACKED memory_map_entry {
        u64 addr;
        u64 size;
        u32 type;
        u32 attrib;
    };

    struct memory_map {
        size_t size;
        memory_map_entry *entries;
    };
}
