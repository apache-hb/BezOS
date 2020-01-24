#include "cpuid.h"

cpu_info_t cpuid(u32 leaf, u32 subleaf)
{
    cpu_info_t info;

    asm volatile("cpuid" 
        : "=a"(info.eax), "=b"(info.ebx), "=c"(info.ecx), "=d"(info.edx) 
        : "a"(leaf), "c"(subleaf)
    );

    return info;
}