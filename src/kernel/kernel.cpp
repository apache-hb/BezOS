#include "vga/vga.h"
#include "mm/mm.h"

#include "sys/idt.h"

extern "C" void kmain(void)
{
    vga::init();

    idt::init();

    // mm::init *must* be called before vesa::init due to the bootloaders 
    // delivery of data
    mm::init();

    vga::puts("hello\n");

    for(;;);
}
