.section .text

.extern KmHalt

.extern KmIsrDispatchRoutine

KmIsrStub:
    push %rbp
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rsi
    push %rdi
    push %rdx
    push %rcx
    push %rbx
    push %rax

    mov %rsp, %rdi
    call KmIsrDispatchRoutine
    mov %rax, %rsp

    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rdi
    pop %rsi
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    pop %rbp

    add $16, %rsp

    iretq

.macro ISR_ENTRY vector
    .align 16
    KmIsrVector\vector:
        pushq \vector
        jmp KmIsrStub
.endm

.macro ISR_ENTRY2 errcode, vector
    .align 16
    KmIsrVector\vector:
        pushq \errcode
        pushq \vector
        jmp KmIsrStub
.endm

.global KmIsrTable
KmIsrTable:
    ISR_ENTRY2 $0, $0
    ISR_ENTRY2 $0, $1
    ISR_ENTRY2 $0, $2
    ISR_ENTRY2 $0, $3
    ISR_ENTRY2 $0, $4
    ISR_ENTRY2 $0, $5
    ISR_ENTRY2 $0, $6
    ISR_ENTRY2 $0, $7
    ISR_ENTRY $8
    ISR_ENTRY2 $0, $9
    ISR_ENTRY $10
    ISR_ENTRY $11
    ISR_ENTRY $12
    ISR_ENTRY $13
    ISR_ENTRY $14
    ISR_ENTRY $15
    ISR_ENTRY2 $0, $16
    ISR_ENTRY $17
    ISR_ENTRY2 $0, $18
    ISR_ENTRY2 $0, $19
    ISR_ENTRY2 $0, $20
    ISR_ENTRY2 $0, $21
    ISR_ENTRY2 $0, $22
    ISR_ENTRY2 $0, $23
    ISR_ENTRY2 $0, $24
    ISR_ENTRY2 $0, $25
    ISR_ENTRY2 $0, $26
    ISR_ENTRY2 $0, $27
    ISR_ENTRY2 $0, $28
    ISR_ENTRY2 $0, $29
    ISR_ENTRY2 $0, $30
    ISR_ENTRY2 $0, $31
    ISR_ENTRY2 $0, $32
