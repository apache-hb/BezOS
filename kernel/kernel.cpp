#include "etc/types.h"

using namespace bezos;

extern "C" void kmain(void)
{
    ((u16*)0xB8000)[0] = 0x0F0F;

    while(1);
}
