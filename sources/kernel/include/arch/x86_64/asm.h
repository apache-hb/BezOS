#ifndef BEZOS_X86_64_ARCH_ASM
#define BEZOS_X86_64_ARCH_ASM

#define PROC_BASE(name) \
    .global name; \
    .type name,@function; \
    .align 16; \
    name:

#define PROC(name) \
    PROC_BASE(name) \
    .cfi_startproc;

#define PROC_SIMPLE(name) \
    PROC_BASE(name) \
    .cfi_startproc simple;

#define ENDP(name) \
    .size name,.-name; \
    .cfi_endproc; \

#endif /* BEZOS_X86_64_ARCH_ASM */
