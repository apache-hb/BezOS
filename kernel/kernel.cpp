#include "types.h"

extern "C" void kmain(void) 
{
    bezos::u16* vga = (bezos::u16*)0xB8000;
    vga[1] = 'a' | ( 1 | 0 << 4) << 8;
}