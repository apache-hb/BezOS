#pragma once

#include "common/types.h"

namespace mm
{
    void init();

    void* alloc(uint64 size);
    void* map(uint64 addr, uint64 len);
    
    void free(void* ptr);
}
