extern descriptor64.data
extern kmain

section .text.front
bits 64
    global start64
    start64:
        ; setup the segments *again*
        cli
        mov ax, descriptor64.data
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        call kmain