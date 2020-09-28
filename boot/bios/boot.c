#include "kernel/kernel.h"

void boot_main() {
    u16 count = *(u16*)(0x7FFFF - 2);
    memory_map_entry_t *entries = 0x7FFFF - (2 + (sizeof(memory_map_entry_t) * count));
    memory_map_t memory = { count, entries };
    kmain(&memory);
}
