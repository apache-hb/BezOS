#include "kernel.h"

const char *str = "yes";

volatile u32 i = 0;

void kmain(memory_map_t memory, u64 *pml4) {
    *((u16*)0xB8000) = 'a' | 7 << 8;
    for (;;) { }
}
