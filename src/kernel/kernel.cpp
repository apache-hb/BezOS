#include "vga/vga.h"
#include "mm/mm.h"

#include "sys/idt.h"
#include "sys/vesa.h"

extern "C" void kmain(void)
{
    vga::init();

    idt::init();

    // mm::init *must* be called before vesa::init due to the bootloaders 
    // delivery of data
    mm::init();
    vesa::init();

    vga::puts("hello\n");

    for(;;);
}
