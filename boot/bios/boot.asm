section .boot

bits 16
prelude:
    cli
    cld
    jmp 0:entry
entry:

boot32:

boot64:


kernel_begin:
    incbin "kernel.bin"
kernel_end:
