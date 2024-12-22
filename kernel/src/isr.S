.section .text

.extern KmHalt

.extern KmIsrDispatchRoutine

KmIsrStub:
    pushq %rbp
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rsi
    pushq %rdi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    mov %rsp, %rdi
    call KmIsrDispatchRoutine
    mov %rax, %rsp

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

    add $16, %rsp

    iretq

.equ ISR_TABLE_STRIDE, 16

.macro ISR_ENTRY vector
    .align ISR_TABLE_STRIDE
    KmIsr\vector:
        pushq \vector
        jmp KmIsrStub
.endm

.macro ISR_ENTRY2 errcode, vector
    .align ISR_TABLE_STRIDE
    KmIsr\vector:
        pushq \errcode
        pushq \vector
        jmp KmIsrStub
.endm

.global KmIsrTable
.align ISR_TABLE_STRIDE
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
    ISR_ENTRY2 $0, $15
    ISR_ENTRY2 $0, $16
    ISR_ENTRY $17
    ISR_ENTRY2 $0, $18
    ISR_ENTRY2 $0, $19
    ISR_ENTRY2 $0, $20
    ISR_ENTRY $21
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
