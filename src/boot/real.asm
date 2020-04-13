bits 16
section .real
    start16:
        jmp $

    times 510 - ($-$$) db 0
    dw 0xAA55