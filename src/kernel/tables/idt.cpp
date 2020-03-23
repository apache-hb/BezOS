#include "idt.h"

#include "common/types.h"

namespace idt
{
    PACKED(idt_entry, {
        uint16 base_low;
        uint16 sel;
        uint8 ist;
        uint8 flags;
        uint16 base_mid;
        uint32 base_high;
        uint32 zero;
    });

    PACKED(idt_ptr, {
        uint16 limit;
        uint32 base;
    });

    void init(void)
    {
        
        //asm volatile("lidt (%0)" :: "r"(ptr) : "memory");
    }
}