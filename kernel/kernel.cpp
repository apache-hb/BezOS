#include "modules/vga/vga.h"

extern "C" void kmain(void) 
{
    bezos::vga::init();
    bezos::vga::print("name jeff");
}