#include "arch/x86_64/asm.h"

/* rdi - context */
PROC(__x86_64_resume)
    movq %rdi, %rsp

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rdi
    popq %rsi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    popq %rbp

    /* Pop the vector number and error code. */
    add $16, %rsp

    iretq
ENDP(__x86_64_resume)
