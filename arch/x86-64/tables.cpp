#include "etc/target.h"

using namespace bezos;

// pointer to first page-map level 4 table
u64* p4t;

extern "C" void init_paging(void)
{
    auto aligned_end = (KERNEL_END + 0x1000 - 1) & ~0x1000;

    p4t = (u64*)aligned_end;

    u64* p3t = (u64*)(aligned_end + 0x1000);
    *p4t = ((u64)p3t | 0b11);

    u64* p2t = (u64*)(aligned_end + 0x2000);
    *p3t = ((u64)p2t | 0b11);
    
    int i = 0;
    while(i != 512)
    {
        // start address is i * 2MB
        u32 addr = i * 1024 * 1024 * 2;
        addr |= 0b100000011;
        p2t[i] = addr;
        i++;
    }
}