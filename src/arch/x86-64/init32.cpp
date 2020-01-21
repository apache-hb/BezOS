#define PREFIX(func) func##_32
#include "common/vga.h"
#include "common/state.h"
#include "common/cpuid.h"

bezos::u8 bezos::vga::column;
bezos::u8 bezos::vga::row;
bezos::u8 bezos::vga::colour;

extern "C" int LOW_MEMORY;

extern "C" void print(const char* msg) { bezos::vga::print_32(msg); }

// export this so assembly code can see it
extern "C" void init_vga() 
{ 
    bezos::vga::column = 0;
    bezos::vga::row = 0;
    bezos::vga::colour = VGA_COLOUR(7, 0);

    // clear vga buffer
    for(int i = 0; i < 80 * 25; i++)
        VGA_BUFFER[i] = VGA_ENTRY(' ', bezos::vga::colour);
        
    bezos::vga::print_32("here\n");
}

using namespace bezos;

struct e820_entry
{
    u64 len;
    u64 addr;
    u32 type;
    u32 attrib;
};

static_assert(sizeof(e820_entry) == 24);

#define PAGE_ALIGN(val) ((val + 0x1000 -1) & -0x1000)

void clear_page(void* ptr)
{
    for(int i = 0; i < 512; i++)
        ((u64*)ptr)[i] = 0;
}

extern "C" void* init_paging()
{
    // parse memory maps
    u32 struct_size = 0;
    u32 offset = LOW_MEMORY;
    u64 total_memory = 0;

    for(;;)
    {
        struct_size = *(u32*)(offset - sizeof(u32));

        if(!struct_size)
            break;

        e820_entry* entry = reinterpret_cast<e820_entry*>(offset - sizeof(bezos::u32) - struct_size);

        if(!entry->len)
        {
            // if the map is 0 bytes long then ignore it
        }

        switch(entry->type)
        {
        case 1: break; // normal usable memory
        case 2: break; // reserved unusable memory
        case 3: break; // ACPI reclaimable memory
        case 4: break; // ACPI reserved memory
        case 5: break; // bad memory
        default: break; // broken memory map
        }

        // if the entry size is 20 then acpi attributes do not exist
        // set them to 0 to avoid any undefined behaviour
        if(struct_size == 20)
            entry->attrib = 0;

        if(entry->attrib & 1)
        {
            // if the first bit is set then we can use this section
            // otherwise this section is unusable
        }

        offset -= (struct_size + 4);
        
        // get the max amount of memory
        if((entry->addr + entry->len) > total_memory)
            total_memory = entry->addr + entry->len;
    }

    // check if pml5 is supported
    bool pml5_supported = cpuid(0x07).ecx & (1 << 16);

    if(pml5_supported)
        vga::print_32("pml5 is supported\n");
    else
        vga::print_32("pml4 is supported\n");

    // align the first page
    u64* page = (u64*)PAGE_ALIGN((u64)&LOW_MEMORY);

    vga::print_32("first page aligned to "); vga::print_32((int)page); vga::print_32("\n");

    if(pml5_supported)
    {
        // if we get here then the cpu has pml5 support
        // we should try and use it if we need it
        // (only on systems with more than 256TiB of ram)
        if(total_memory > 281474976710656ULL)
        {
            // we actually need the pml5 here
        }
    }

    auto* pml4 = page;
    page += 0x1000;
    auto* pml3 = page;
    page += 0x1000;
    auto* pml2 = page;
    page += 0x1000;
    auto* pt = page;

    pml4[0] = (u64)pml3 | 3;
    pml3[0] = (u64)pml2 | 3;
    pml2[0] = (u64)pt | 3;

    vga::print_32((int)pt); print("\n");
    vga::print_32((int)pml2); print("\n");
    vga::print_32((int)pml3); print("\n");
    vga::print_32((int)pml4); print("\n");

    // map the first 2mb to original address
    for(int i = 0; i < 512; i++)
    {
        pt[i] = (i * 0x1000) | 3;
    }

    vga::print_32("here 2\n");

    return pml4;
}
