extern setup_paging

extern kmain

extern TOP_PAGE

section .protected
bits 32
    global start32
    start32:
        ; we need to do segmentation because protected mode needs it
        pop ax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov ss, ax
        mov gs, ax


        ; check eflags to see if we have cpuid
        pushfd
        pop eax

        mov ecx, eax

        xor eax, 1 << 21

        push eax
        popfd

        pushfd
        pop eax

        push ecx
        popfd

        xor eax, ecx
        jz .no_cpuid

        ; check if we have extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .no_long

        ; check if we have long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz .no_long

        ; lets enable floating point numbers as well
        ; those seem important
        mov eax, cr0
        and eax, (-1) - ((1 << 2) | (1 << 3))
        mov cr0, eax
        ; enable the fpu unit
        fninit
        fnstsw [fpu_test]
        cmp word [fpu_test], 0 ; if nothing is written then we dont have an fpu unit
        jne .no_fpu

        ; lets enable sse and sse2 now
        mov eax, 0x1
        cpuid 
        test edx, 1 << 25
        jz .no_sse

        mov eax, cr0
        ; disable emulation bit
        and eax, ~(1 << 2)
        ; enable co proccessor bit
        or eax, (1 << 1)
        mov cr0, eax

        ; enable the fxsave/restore functions and sse exceptions
        mov eax, cr4
        or eax, (1 << 9) | (1 << 10)
        mov cr4, eax

        ; now we setup paging
        call setup_paging
    
    ; if we are here we dont have cpuid
    .no_cpuid:

    ; if we are here we dont have extended cpuid and therefore no long mode
    .no_extended_cpuid:

    ; if we got here we dont have long mode
    .no_long:

    ; if we get here we have no fpu unit (but protected mode)
    ; this is either one funky cpu or its buggy
    .no_fpu:

    ; if we get here we dont support sse
    .no_sse:

    ; data to test the fpu with
    fpu_test: dw 0xFFFF