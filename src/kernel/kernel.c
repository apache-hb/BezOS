#include "vga/vga.h"
#include "mm/mm.h"

#include "tables/gdt.h"
#include "tables/idt.h"

extern void kmain(void)
{
    vga_init();

    gdt_init();

    idt_init();

    mm_init();

    vga_print("hello\n");

    for(;;);
}
