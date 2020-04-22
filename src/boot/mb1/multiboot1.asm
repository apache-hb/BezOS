align 4
section .multiboot
    magic: dd 0x1BADB002
    flag: 1 << 0 | 1 << 1
    checksum: -(magic + flag)

section .bss
    align 16

    resb 1024 * 16
    stack:

bits 32
global kmain
section .text
    mov esp, stack

    