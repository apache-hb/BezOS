extern kmain

global start64

extern GDT64

bits 64
section .long
    start64:
        jmp $
        hlt

        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
    
        lgdt [GDT64]

        jmp main

    main:

        jmp $

        movsx rsp, esp
        mov rbp, rsp
        call kmain