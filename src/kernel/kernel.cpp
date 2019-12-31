#include "common/types.h"

using namespace bezos;

extern "C" u32* p4t = 0;

extern "C" void kmain(void)
{
    ((u16*)0xB8000)[0] = 0x0F0F;

    while(1);
}
