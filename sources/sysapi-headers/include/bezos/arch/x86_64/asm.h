#ifndef BEZOS_X86_64_ARCH_ASM
#define BEZOS_X86_64_ARCH_ASM

#define PROC(name) \
    .global name; \
    .type name,@function; \
    .align 16; \
    name: \
    .cfi_startproc;

#define ENDP(name) \
    .cfi_endproc; \
    .size name,.-name;

#endif /* BEZOS_X86_64_ARCH_ASM */
