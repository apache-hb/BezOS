#include "common/types.h"

extern "C" void kmain(void)
{
    ((u16*)0xB8000)[0] = 0xFFFF;
}