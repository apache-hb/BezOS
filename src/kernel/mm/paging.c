#include "paging.h"

#include "hw/registers.h"

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

static void* get_virtual_address()
{

}

void paging_init()
{
    // get the total amount of memory we have
    const u64 total_memory = memory_total();

    // check if we support pml5 or not
    const u32 pml5_support = cpuid(0x07, 0).ecx & PML5_FLAG;
}

void* page_alloc(u64 num, page_size_t size)
{
    // return alloc_proc(num, size);
    return 0;
}

void* map_physical_address(void* addr, page_modifier_t mods)
{
    u64 phys = (u64)addr;
    
    if(phys % 0x1000)
    {
        // if the address isnt page aligned return null
        return 0;
    }

    phys |= mods;
    
    return 0;
}

#if 0

typedef void*(*page_alloc_proc_t)(u64, page_size_t);

static union {
    pml5_t lvl5;
    pml4_t lvl4;
} top_page;

static page_alloc_proc_t alloc_proc;

extern u32 LOW_MEMORY;
extern u32 KERNEL_END;

static pt_t* alloc_pt(addr_t at)
{
    pt_t pt = at;
    memset(pt, 512, 0);
    return pt;
}

static pml2_t alloc_pml2(addr_t at)
{
    pml2_t pml = at;
    memset(pml, 512, 0);
    return pml;
}

static pml3_t alloc_pml3(addr_t at)
{
    pml3_t pml = at;
    memset(pml, 512, 0);
    return pml;
}

static pt_t alloc_pt(addr_t at)
{
    pt_t pt = at;
    memset(pt, 512, 0);
    return pt;
}

static void* alloc_pml4(u64 num, page_size_t size)
{
    return 0;
}

static void* alloc_pml5(u64 num, page_size_t size)
{
    return 0;
}
#endif

#if 0

    pml4_t pml4 = 0;

    if(PETABYTE(2ULL) < total_memory && pml5_support)
    {
        // using pml5 here as we have it and need it
        top_page.lvl5 = (pml5_t)PAGE_ALIGN(LOW_MEMORY);
        alloc_proc = alloc_pml5;

        pml5_t pml5 = top_page.lvl5;
        memset(pml5, 512, 0);
        pml4 = pml5 + 0x1000;
        memset(pml4, 512, 0);
        pml5[0] = pml4 | 3;
    }
    else
    {
        // we dont need pml5
        top_page.lvl4 = (pml4_t)PAGE_ALIGN(LOW_MEMORY);
        alloc_proc = alloc_pml4;
        pml4 = top_page.lvl4;
        memset(pml4, 512, 0);
    }

    // here the pml4 is zeroed so we have to populate it
    i32 end = KERNEL_END;

    pml3_t pml3 = alloc_pml3(pml4 + 0x1000);
    pml4[0] = pml3 | 3;

    pml2_t pml2 = alloc_pml2(pml3 + 0x1000);
    pml3[0] = pml2 | 3;

    int i = 0;
    while(end > 0)
    {
        pt_t pt = alloc_pt(pml2 + (i * 0x1000));
        
        for(int j = 0; j < 512; j++)
        {
            pt[j] = ((j * 0x1000) + (i * MEGABYTE(2))) | 3;
            end -= 0x1000;
        }

        pml2[i++] = pt;
    }


    // TODO: set cr3
#endif