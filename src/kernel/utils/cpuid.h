#ifndef UTILS_CPUID_H
#define UTILS_CPUID_H 1

#include "common/types.h"

typedef struct {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
} cpu_info_t;

cpu_info_t cpuid(u32 leaf, u32 subleaf);

#define PML5_FLAG (1 << 16)

#endif // UTILS_CPUID_H
