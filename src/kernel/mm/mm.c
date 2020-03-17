#include "mm.h"
#include "vga/vga.h"

#include "common/types.h"

extern uint64 LOW_MEMORY;

typedef struct {
    uint64 addr;
    uint64 size;
    uint32 type;
    uint32 attrib;
} e820_entry;

void mm_init(void)
{
    byte* ptr = (byte*)LOW_MEMORY;

    for(;;)
    {
        
    }
}
