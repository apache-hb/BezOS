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
bits 32
    global _boot:function (_boot.end - _boot)
    _boot:
        ; make sure we have cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        ; if we dont this cpu is bad and we wont boot
        jb unsupported

        ; if we do have cpuid then check if we have long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        ; if we dont then this cpu is bad
        jz unsupported

        mov esp, stack_front

        extern kmain
        call kmain

        cli
    .halt: hlt
        jmp .halt
    .end:

    unsupported:
        mov [0xB8000], 0x30

        cli
        hlt
