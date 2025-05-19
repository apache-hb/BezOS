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
    .cfi_endproc;

#define PUSHQ_CFI_REG_ALIAS(reg, alias) \
    pushq reg; \
    .cfi_adjust_cfa_offset 8; \
    .cfi_rel_offset alias, 0;

#define POPQ_CFI_REG_ALIAS(reg, alias) \
    popq reg; \
    .cfi_adjust_cfa_offset -8; \
    .cfi_restore alias;

#define PUSHQ_CFI_REG(reg) PUSHQ_CFI_REG_ALIAS(reg, reg)
#define POPQ_CFI_REG(reg) POPQ_CFI_REG_ALIAS(reg, reg)

#endif /* BEZOS_X86_64_ARCH_ASM */
