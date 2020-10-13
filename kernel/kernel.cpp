#include <kernel.h>

#include <log/log.h>

SECTION(".kmain")
extern "C" void kmain(pmm::memory_map memory, vmm::pml4 pml4) {
    ((u16*)0xB8000)[0] = 'd' | 7 << 8;
    log::vga out;
    log::init(&out);

    ((u16*)0xB8000)[0] = 'e' | 7 << 8;

    log::info("hello\n");

    ((u16*)0xB8000)[0] = 'f' | 7 << 8;
    //out.info("hello\n");
    // map the bss, data, text, and rodata sections here
    // then init the mm, idt, pci, smp, etc
    // then do something here
    for (;;) { }
}
