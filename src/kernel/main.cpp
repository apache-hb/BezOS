#include "types.h"

extern "C" {
    int BSS_ADDR;
    int BSS_SIZE;
}

void clear_bss()
{
    uint64* bss = (uint64*)&BSS_ADDR;
    const uint64 size = reinterpret_cast<uint64>(&BSS_SIZE);

    for(int i = 0; i < size / 8; i++)
        bss[i] = 0;
}

extern "C" void kmain()
{
    clear_bss();
}