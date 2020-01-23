#include "memory.h"

#include "common/types.h"

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

void memory_init(void)
{
    u32 count = 0;
    u32 size = 0;
    u32 usable_size = 0;
    void* ptr = (void*)&LOW_MEMORY;

    for(;;)
    {
        u32 entry_size = *(u32*)ptr - sizeof(u32);

        // last entry size will be 0
        if(!entry_size)
            break;

        e820_entry* entry;
    }
}