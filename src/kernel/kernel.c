#include "vga/vga.h"
#include "mm/mm.h"

extern void kmain(void)
{
    vga_init();
    vga_print("hello\n");
    mm_init();

    for(;;);
}
