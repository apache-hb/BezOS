#include "mm.h"
#include "vga/vga.h"

extern "C" uint64 LOW_MEMORY;
extern "C" uint64 KERNEL_END;

namespace mm
{
    struct e820_entry 
    {
        uint64 addr;
        uint64 size;
        uint32 type;
        uint32 attrib;
    };

    static e820_entry* entries;
    static int entry_count;

    void init(void)
    {
        byte* ptr = (byte*)LOW_MEMORY;
        entries = (e820_entry*)LOW_MEMORY;
        entry_count = 0;

        // go backwards over all the entries starting at LOW_MEMORY-4
        for(;;)
        {
            uint32 size = *(uint32*)(ptr - sizeof(uint32));

            if(!size)
                break;

            ptr -= (sizeof(uint32) + size);

            e820_entry* entry = (e820_entry*)ptr;

            if(size == 20)
                entry->attrib = 0;

            // put the entries into their own table for later use
            entries[entry_count++] = *entry;
        }
    }
}