.global KmResumeThread

.section .text

/* Resume a thread from a context. */
/* rdi: pointer to the context */
KmResumeThread:
    mov %rdi, %rsp

    /* Restore general registers */
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
    /* We reuse the IsrContext struct for the thread context. */
    add $16, %rsp

    iretq
