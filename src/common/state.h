#pragma once

#include "types.h"

// manages global kernel state
namespace bezos
{
    extern "C" {
        // the size of the kernel aligned to next page
        int KERNEL_END;
        
        // the number of 512 byte sectors the kernel takes up
        int KERNEL_SECTORS;
    }
}