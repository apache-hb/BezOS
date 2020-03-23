#include "vga/vga.h"
#include "mm/mm.h"

#include "tables/idt.h"

extern "C" void kmain(void)
{
    vga::init();

    idt::init();

    mm::init();

    vga::print("hello\n");

    for(;;);
}
