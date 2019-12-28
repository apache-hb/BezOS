; protected mode assembly

global start32:function
extern kmain
extern kpanic

section .protected
; we're in protected mode now, so we can use 32 bit instructions
bits 32
    ; 32 bit start
    start32:
        mov bx, 0x10
        mov ds, bx
        mov es, bx

        mov al, 0xFE
        mov cr0, eax

        pop es
        pop ds
        sti
        ; check for long mode so we can get into it
        ; use cpuid to check for long mode

        ; check if we have cpuid

        ; copy flags to eax
        pushfd
        pop eax

        ; move flags to ecx for later
        mov ecx, eax

        ; flip the id bit
        xor eax, 1 << 21

        ; set flags to eax
        push eax
        popfd

        ; set eax to flags. the bit will be flipped if cpuid is supported
        pushfd
        pop eax

        ; restore flags to ecx
        push ecx
        popfd

        ; if eax and ecx arent equal then we dont have cpuid
        xor eax, ecx
        jz .cpuid_no

        ; check if we support extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .long_no

        ; if we support extended cpuid we can check if we support long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz .long_no

    .long_on:
        call kmain

    .cpuid_no:
        call kpanic
    .long_no:
        call kpanic
    panic:
