#include "memory.h"

#include "kernel/types.h"

#define E820_ENTRIES 128

namespace bezos::memory
{
    struct e820_entry
    {
        // base physical address
        addr_t base;

        // length of segment
        u64 length;

        // type of segment
        u32 type;

        // acpi attributes, may be 0
        u32 flags;
    };

    static_assert(sizeof(e820_entry) == 24);

    e820_entry entries[E820_ENTRIES];

    u32 detect_memory_e820()
    {
        int count = 0;
        e820_entry buf = {};

        u32 ebx, eax, eflags;

        while(ebx && count < E820_ENTRIES) 
        {
            return 0;
            // asm volatile(
            //     "xor ebx, ebx\n" // clear ebx (must be 0 for smap to work)
            //     "mov $0x0534D4150, %%eax\n" // put "SMAP" in eax
            //     "mov $0xE820, eax\n" // put e820 into eax
            //     "int 0x15"
            // );
        }
        return 0;
    }

    void init()
    {

    }
}