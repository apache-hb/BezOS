#include <kernel.h>

int i = 0;

extern "C" void kmain(mm::memory_map memory, mm::pml4 pml4) {
    ((u16*)0xB8000)[1] = 'a' | 7 << 8;
    // map the bss, data, text, and rodata sections here
    // then init the mm, idt, pci, smp, etc
    // then do something here
    for (;;) { 
        i = ~i;
    }
}
