#include "vga/vga.h"

extern void kmain(void)
{
    vga_init();
    vga_print("hello\n");
    while(1);
}
