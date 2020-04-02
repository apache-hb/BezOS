#include "vga/vga.h"
#include "mm/mm.h"

#include "sys/idt.h"

extern "C" void kmain(void)
{
    vga::init();

    idt::init();
    mm::init();

    vga::puts("hello\n");
    
    for(;;);
}
