#include "arch/x86_64/asm.h"

#define REG_R8 0
#define REG_R9 1
#define REG_R10 2
#define REG_R11 3
#define REG_R12 4
#define REG_R13 5
#define REG_R14 6
#define REG_R15 7
#define REG_RDI 8
#define REG_RSI 9
#define REG_RBP 10
#define REG_RBX 11
#define REG_RDX 12
#define REG_RAX 13
#define REG_RCX 14
#define REG_RSP 15
#define REG_RIP 16

.section .text

// extern "C" [[noreturn]] void PvTestContextSwitch(gregset_t *gregs);
PROC(PvTestContextSwitch)
    // Save the RIP to the top of the new stack
    movq (REG_RIP * 8)(%rdi), %rcx
    movq (REG_RSP * 8)(%rdi), %rdx
    subq $8, %rdx
    movq %rcx, (%rdx)

    movq (REG_R8 * 8)(%rdi), %r8
    movq (REG_R9 * 8)(%rdi), %r9
    movq (REG_R10 * 8)(%rdi), %r10
    movq (REG_R11 * 8)(%rdi), %r11
    movq (REG_R12 * 8)(%rdi), %r12
    movq (REG_R13 * 8)(%rdi), %r13
    movq (REG_R14 * 8)(%rdi), %r14
    movq (REG_R15 * 8)(%rdi), %r15
    movq (REG_RSI * 8)(%rdi), %rsi
    movq (REG_RBP * 8)(%rdi), %rbp
    movq (REG_RBX * 8)(%rdi), %rbx
    movq (REG_RDX * 8)(%rdi), %rdx
    movq (REG_RAX * 8)(%rdi), %rax
    movq (REG_RCX * 8)(%rdi), %rcx
    movq (REG_RSP * 8)(%rdi), %rsp
    movq (REG_RDI * 8)(%rdi), %rdi

    subq $8, %rsp

    retq
ENDP(PvTestContextSwitch)
