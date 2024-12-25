#include "gdt.hpp"

#include "arch/intrin.hpp"

void KmInitGdt(const uint64_t *gdt, uint64_t count, uint64_t codeSelector, uint64_t dataSelector) {
    GDTR gdtr = {
        .limit = uint16_t((count * sizeof(uint64_t)) - 1),
        .base = (uint64_t)gdt
    };

    __lgdt(gdtr);

    asm volatile (
        // long jump to reload the CS register with the new code segment
        "pushq %[code]\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:"
        : /* no outputs */
        : [code] "r"(codeSelector * sizeof(uint64_t))
        : "memory", "rax"
    );

    asm volatile (
        // zero out all segments aside from ss
        "mov $0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        // load the data segment into ss
        "movw %[data], %%ax\n"
        "mov %%ax, %%ss\n"
        : /* no outputs */
        : [data] "r"((uint16_t)(dataSelector * sizeof(uint64_t))) /* inputs */
        : "memory", "rax" /* clobbers */
    );
}
