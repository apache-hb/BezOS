#include "common/types.h"
#include "common/vga.h"

extern "C" void kmain(void)
{
    bezos::u64 i = *(bezos::u64*)0xB8000;
    i++;
    bezos::vga::print(i);
    bezos::vga::print("64 bit\n");
    while(true);
}