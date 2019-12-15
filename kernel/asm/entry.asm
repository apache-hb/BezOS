global _start

extern kmain

section .text
bits 32

_start:
    mov esp, 0xeFFFF0