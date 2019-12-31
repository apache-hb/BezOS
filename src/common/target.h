#pragma once

#include "types.h"

#ifndef TARGET_X86
#   define TARGET_X86 0
#endif

#ifndef TARGET_ARM
#   define TARGET_ARM 0
#endif

#if TARGET_X86
namespace bezos
{
    // page table entry
    using pte = u32;

    // page table
    using pt = pte[1024];
    
    // page directory table
    // using pd = u32[1024];

    // page directory pointer table
    // using pdp = pd[1024];

    // page map level 4
    // using pml4 = pdp[1024];

    // page map level 5 (only supported by 10th gen intel right now)
    // using pml5 = pml4*[1024];
}

// page map level 4 table
extern "C" bezos::u64* p4t;

#endif

extern "C" bezos::u64 KERNEL_END;