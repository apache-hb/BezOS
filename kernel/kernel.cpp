#include "metal/vga/vga.h"
#include "metal/ram/ram.h"

extern "C" void kmain()
{
    bezos::vga::init();
    bezos::ram::init();
}