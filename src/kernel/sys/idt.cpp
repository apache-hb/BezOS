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

    idt_entry entry(void* func, uint8 ist, uint8 type)
    {
        uint64 offset = reinterpret_cast<uint64>(func);

        idt_entry entry = {};
        
        entry.base_low = static_cast<uint16>(offset);
        entry.base_mid = static_cast<uint16>(offset >> 16);
        entry.base_high = static_cast<uint32>(offset >> 32);
        
        entry.ist = ist;
        entry.sel = 0x8;
        entry.flags = type;
        entry.zero = 0;

        return entry;
    }

    PACKED(idt_ptr, {
        uint16 limit;
        uint64 base;
    });

    idt_entry entries[256];

    void init(void)
    {
        for(auto& each : entries)
            each = entry(nullptr, 0, 0x8E);

        idt_ptr iptr = { sizeof(entries) - 1, reinterpret_cast<uint64>(entries) };
        
        asm volatile("lidt %0" :: "m"(iptr));
    }
}