#include "common/types.h"

using namespace bezos;

extern "C" void thing(void)
{
    ((u16*)0xB8000)[0] = 0xFFFF;

    while(true);
}