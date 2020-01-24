#include "paging.h"

#include "memory.h"
#include "vga/vga.h"
#include "utils/cpuid.h"
#include "utils/convert.h"

typedef u64 addr_t;
typedef addr_t* pt_t;
typedef pt_t* pml2_t;
typedef pml2_t* pml3_t;
typedef pml3_t* pml4_t;
typedef pml4_t* pml5_t;

static union {
    pml5_t lvl5;
    pml4_t lvl4;
} top_page;

static u8 page_level;

extern u32 LOW_MEMORY;

void paging_init()
{
    vga_puts("paging_init");

    u64 total_memory = memory_total();

    u32 pml5_support = cpuid(0x07, 0).ecx & PML5_FLAG;

    if(PETABYTE(2ULL) < total_memory && !pml5_support)
    {
        // warn about memory
    }

    if(PETABYTE(2ULL) < total_memory && pml5_support)
    {
        top_page.lvl5 = (pml5_t)PAGE_ALIGN(LOW_MEMORY);
    }
    else
    {
        // we dont need pml5
        top_page.lvl4 = (pml4_t)PAGE_ALIGN(LOW_MEMORY);
        page_level = 4;
    }
}

void* page_alloc(u64 num, page_size_t size)
{
    return (void*)0;
}