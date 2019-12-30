#include "etc/types.h"

extern u64 KERNEL_END;

void* pml4;

extern "C" void init_paging(void)
{
    auto aligned_end = (KERNEL_END + 0x1000 - 1) & ~0x1000;

    ((u16*)0xB8000)[0] = aligned_end == 0x1000 ? 0xFFFF : 0x0F0F;

    while(true);
}