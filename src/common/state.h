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

// we have to decide where to put the first page in virtual memory
// this is a bit of a painful global but is required
// TODO: maybe remove this or if it turns out to be good then leave as is
#define FIRST_PAGE_OFFSET 0x1000 
// set it to the second page since the first will be
// no read/write/exec so segfaults can be detected