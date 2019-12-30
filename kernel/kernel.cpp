#include "etc/types.h"

extern "C" void kmain(void)
{
    volatile u16* vga = (u16*)0xB8000;
    for(int i = 0; i < 80 * 25; i++)
        vga[i] = 25;
    
    while(1);
}
