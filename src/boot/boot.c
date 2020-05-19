#include "boot.h"

#if defined(__x86_64__)
#   error "code must be compiled for 32 bit"
#endif

typedef struct {
    u64 memory;
    u32 parts;
} bootinfo32_t;

extern bootinfo32_t* bootinfo;

extern u32 BSS32_START;
extern u32 BSS32_END;

struct {
    u64 pml4[512];
    u64 pml3[512];
    u64 pml2[512];
    u64 pml1[512];
} tables __attribute__((aligned(0x1000)));

void* init32()
{   
    // zero the bss for the 32 bit code
    for(u32 i = (u32)&BSS32_START; i < (u32)&BSS32_END; i++)
        *((u32*)i) = 0;

    for(int i = 0; i < 512; i++)
        tables.pml1[i] = (i * 0x1000) | 0x3;

    tables.pml2[0] = (u64)tables.pml1 | 0x3;
    tables.pml3[0] = (u64)tables.pml2 | 0x3;
    tables.pml4[0] = (u64)tables.pml3 | 0x3;

    return tables.pml4;
}