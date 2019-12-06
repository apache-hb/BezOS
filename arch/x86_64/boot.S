#define MB_MAGIC    0x1badb002
#define MB_FLAGS    1<<0 | 1<<1
#define MB_CHECK    (-(MB_MAGIC + MB_FLAGS))

#define KERNEL_LMA  0x0000000000200000
#define KERNEL_VMA  0xFFFFFFFF80000000
#define ABSOLUTE(x) ((x) - KERNEL_VMA + KERNEL_LMA)

.global _boot
.type _boot, @function

.extern _kernel_bsp, _kernel_ap
.type _kernel_bsp, @function
.type _kernel_ap, @function

.section .boot, "ax"
    .code32

    .balign 4, 0
    multiboot_header:
        .long MB_MAGIC
        .long MB_FLAGS
        .long MB_CHECK

    .balign 8, 0
    _boot:
        cli

        movl %eax, (mb_eax)
        movl %ebx, (mb_ebx)

        movl %cr0, %eax
        andl $0x7FFFFFFF, %eax
        movl %eax, %cr0

        movl $ABSOLUTE(pml4), %edi
        movl %edi, %cr3

        movl %cr4, %eax
        orl $(1 << 5), %eax
        movl %eax, %cr4

        movl $0xC0000080, %ecx
        rdmsr
        orl $(1 << 8), %eax
        wrmsr

        movl %cr0, %eax
        orl $(1 << 31), %eax
        movl %eax, %cr0

        lgdt gdt64_ptr
        jmp $8, $boot64_low

    .code64
    boot64_low:
        movw $0x10, %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %fs
        movw %ax, %gs
        movw %ax, %ss

        xorq %rax, %rax
        movl $0xC0000100, %ecx
        wrmsr
        movl $0xC0000101, %ecx
        wrmsr

        jmp boot64_high

    die64_low:
        hlt
        jmp die64_low

    .balign 16
    gdt64:
        .quad 0
        
        .word 0
        .word 0
        
        .byte 0
        .byte 0x98 
        .byte 0xA0
        .byte 0
        
        .word 0
        .word 0

        .byte 0
        .byte 0x92
        .byte 0xC0
        .byte 0

    gdt64_ptr:
        .word 3 * 8 - 1
        .quad gdt64

    mb_eax:
        .long 0
    mb_ebx:
        .long 0

.section .init.text, "ax"
    .code64

    boot64_high:
        movq $boot_stack_front, %rsp
        xorq %rbp, %rbp

        xorq %rax, %rax
        pushq %rax
        popfq

        movl (mb_eax), %eax
        movl (mb_ebx), %edi
        pushq $0
        cmpl $0x2BADB002, %eax
        je _kernel_bsp
        cmpl $0xDEADBEEF, %eax
        je _kernel_ap

    die64_high:
        hlt
        jmp die64_high

.section .init.data, "aw"
    .balign 0x1000
    pml4:
        .quad ABSOLUTE(pdp0) + 7
        .skip 255 * 8
        .quad ABSOLUTE(pdp0) + 7
        .skip 254 * 8
        .quad ABSOLUTE(pdp1) + 7

    .balign 0x1000
    pdp0:
        .quad ABSOLUTE(pd + 0x0000) + 7
        .quad ABSOLUTE(pd + 0x1000) + 7
        .quad ABSOLUTE(pd + 0x2000) + 7
        .quad ABSOLUTE(pd + 0x3000) + 7

    .balign 0x1000
    pdp1:
        .skip 510 * 8
        .quad ABSOLUTE(pd + 0x0000) + 7
        .quad ABSOLUTE(pd + 0x1000) + 7

    .balign 0x1000
    pd:
    index = 0
    .rept 512 * 4
        .quad (index << 21) + 0x87
        index = index + 1
    .endr

    .balign 0x1000
    boot_stack:
        .skip 0x4000
    boot_stack_front: