#pragma once

#include <util.h>

namespace pmm {
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

    void init(memory_map map);
}