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
    // base interrupt vectors
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

    // apic interrupt vectors
    ISR_ENTRY2 $32, $32
    ISR_ENTRY2 $33, $33
    ISR_ENTRY2 $34, $34
    ISR_ENTRY2 $35, $35
    ISR_ENTRY2 $36, $36
    ISR_ENTRY2 $37, $37
    ISR_ENTRY2 $38, $38
    ISR_ENTRY2 $39, $39
    ISR_ENTRY2 $40, $40
    ISR_ENTRY2 $41, $41
    ISR_ENTRY2 $42, $42
    ISR_ENTRY2 $43, $43
    ISR_ENTRY2 $44, $44
    ISR_ENTRY2 $45, $45
    ISR_ENTRY2 $46, $46
    ISR_ENTRY2 $47, $47
    ISR_ENTRY2 $48, $48
    ISR_ENTRY2 $49, $49
    ISR_ENTRY2 $50, $50
    ISR_ENTRY2 $51, $51
    ISR_ENTRY2 $52, $52
    ISR_ENTRY2 $53, $53
    ISR_ENTRY2 $54, $54
    ISR_ENTRY2 $55, $55
    ISR_ENTRY2 $56, $56
    ISR_ENTRY2 $57, $57
    ISR_ENTRY2 $58, $58
    ISR_ENTRY2 $59, $59
    ISR_ENTRY2 $60, $60
    ISR_ENTRY2 $61, $61
    ISR_ENTRY2 $62, $62
    ISR_ENTRY2 $63, $63
