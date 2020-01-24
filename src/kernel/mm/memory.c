#include "memory.h"

#include "common/types.h"

#include "vga/vga.h"

extern int LOW_MEMORY;

typedef struct {
    // physical address
    u64 addr;
    // length of address range
    u64 len;
    // type of memory
    u32 type;
    // extended attributes for acpi3
    u32 attrib;
} e820_entry;   

static u64 total_mem;
static u64 total_usable_mem;

void memory_init(void)
{
    u8* ptr = (u8*)LOW_MEMORY;

    vga_puti((int)ptr, 16);

    // TODO: handle with overlapping memory regions returned by e820
    // TODO: save usable memory ranges to prevent janky stuff
    for(;;)
    {
        u32 entry_size = *(u32*)(ptr - sizeof(u32));

        vga_puti(entry_size, 10);

        // last entry size will be 0
        if(!entry_size)
            break;

        ptr -= sizeof(u32) + entry_size;

        e820_entry* entry = (e820_entry*)ptr;

        if(entry_size == 20)
            entry->attrib = 0;

        if(entry->type == 1)
            total_usable_mem += entry->len;

        total_mem += entry->len;
    }
}

u64 memory_total(void)
{
    return total_mem;
}

u64 memory_total_usable(void)
{
    return total_usable_mem;
}