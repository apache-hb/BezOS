#include "mm/memory.h"
#include "mm/paging.h"

#include "vga/vga.h"

extern void kmain(void)
{
    vga_init();
    vga_print("hello\n");
    // memory_init();
    // paging_init();

    while(1);
}
