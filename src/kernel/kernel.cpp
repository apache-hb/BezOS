#include "common/types.h"
#include "common/vga.h"

extern "C" void kmain(void)
{
    bezos::vga::print("64 bit nibba\n");
    while(true);
}