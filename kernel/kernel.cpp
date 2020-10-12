#include <kernel.h>

extern "C" void kmain(pmm::memory_map memory, vmm::pml4 pml4) {
    char str[] = "hello";
    for (int i = 0; i < sizeof(str); i++) {
        ((u16*)0xB8000)[i] = str[i] | 7 << 8;
    }

    // map the bss, data, text, and rodata sections here
    // then init the mm, idt, pci, smp, etc
    // then do something here
    for (;;) { }
}
