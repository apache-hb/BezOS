#pragma once

#include "common/types.h"

namespace bezos
{
    void copy(void* to, void* from, u64 len)
    {
        for(u64 i = 0; i < len; i++)
            ((byte*)to)[i] = ((byte*)from)[i];
    }
}