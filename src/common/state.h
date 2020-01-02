#pragma once

#include "types.h"


// manages global kernel state
namespace bezos
{
    extern "C" {
        // the size of the kernel aligned to next page
        u32 KERNEL_END;
        // the number of 512 byte sectors the kernel takes up
        u32 KERNEL_SECTORS;
        // the pointer to the top level page map, either level4 or level5
        byte* TOP_PAGE;

        // will either be 4 or 5 depending if pml5 supported or not
        u8 MEMORY_LEVEL;
    }
}