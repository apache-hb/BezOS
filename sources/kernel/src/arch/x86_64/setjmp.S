#include "arch/x86_64/asm.h"

.global __x86_64_setjmp
.global __x86_64_longjmp

.section .text

#define RBX_OFFSET (0*8)
#define RBP_OFFSET (1*8)
#define R12_OFFSET (2*8)
#define R13_OFFSET (3*8)
#define R14_OFFSET (4*8)
#define R15_OFFSET (5*8)
#define RSP_OFFSET (6*8)
#define RIP_OFFSET (7*8)

/* rdi - context */
PROC(__x86_64_setjmp)
    movq %rbx, RBX_OFFSET(%rdi)
    movq %rbp, RBP_OFFSET(%rdi)
    movq %r12, R12_OFFSET(%rdi)
    movq %r13, R13_OFFSET(%rdi)
    movq %r14, R14_OFFSET(%rdi)
    movq %r15, R15_OFFSET(%rdi)

    /* Save the stack pointer of the caller */
    lea 8(%rsp), %rax
    movq %rax, RSP_OFFSET(%rdi)

    /* Save the return address */
    movq 0(%rsp), %rax
    movq %rax, RIP_OFFSET(%rdi)

    xorq %rax, %rax
    ret
ENDP(__x86_64_setjmp)

/* rdi - context
   rsi - value */
PROC(__x86_64_longjmp)
    movq RBX_OFFSET(%rdi), %rbx
    movq RBP_OFFSET(%rdi), %rbp
    movq R12_OFFSET(%rdi), %r12
    movq R13_OFFSET(%rdi), %r13
    movq R14_OFFSET(%rdi), %r14
    movq R15_OFFSET(%rdi), %r15
    movq RSP_OFFSET(%rdi), %rsp

    /* Load return address to jump to */
    movq RIP_OFFSET(%rdi), %rcx

    /* Set return value */
    movq %rsi, %rax

    /* Jump to the return address */
    jmpq *%rcx
ENDP(__x86_64_longjmp)
