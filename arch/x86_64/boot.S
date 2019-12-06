#define MB_MAGIC    0x1badb002
#define MB_FLAGS    1<<0 | 1<<1
#define MB_CHECK    (-(MB_MAGIC + MB_FLAGS))

#define KERNEL_LMA  0x0000000001000000
#define KERNEL_VMA  0xFFFFFFFF81000000
#define ABSOLUTE(x) ((x) - KERNEL_VMA + KERNEL_LMA)

.global _boot
.type _boot, @function

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

    .code64
    boot64_low:
        nop

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
        nop

    die64_high:
        hlt
        jmp die64_high

.section .init.data, "aw"
    .balign 0x1000