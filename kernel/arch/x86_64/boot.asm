MAGIC equ 0x1BADB002
PAGEALIGN equ 1 << 0
MEMINFO equ 1 << 1
FLAGS equ (PAGEALIGN | MEMINFO)
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
    stack_back:
        resb 1024 * 16
    stack_front:

section .text
    global _boot:function (_boot.end - _boot)
    _boot:
        mov esp, stack_front

        extern kmain
        call kmain

        cli
    .halt: hlt
        jmp .halt
    .end: