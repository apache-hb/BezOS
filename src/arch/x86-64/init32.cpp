#include "common/vga.h"
#include "common/state.h"
#include "common/cpuid.h"

extern "C" void print(const char* msg) { bezos::vga::print(msg); }

// export this so assembly code can see it
extern "C" void init_vga() { bezos::vga::init(); }

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

extern "C" void* init_paging()
{
    // parse memory maps
    u32 struct_size = 0;
    u32 offset = LOW_MEMORY;
    u32 total_memory = 0;

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
    bool pml5_supported = cpuid(0x07, 0).ecx & (1 << 16);

    if(pml5_supported)
        vga::print("pml5 is supported\n");
    else
        vga::print("pml4 is supported\n");

    // align the first page
    u64* page = (u64*)PAGE_ALIGN(LOW_MEMORY);

    vga::print("first page aligned to "); vga::print((int)page); vga::print("\n");

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
    pt[0] = 0x0000;

    return nullptr;
}
