; protected mode assembly

global start32:function

; 64 bit kernel main function
extern kmain

; the end of the binary
extern KERNEL_END

extern init_paging

section .protected
bits 32
    start32:
        pop ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov fs, ax
        mov gs, ax

        ; before we continue lets init some stuff like vga output for easier debugging
        ; call kmain32

        ; we need to check if cpuid exists
        
        ; copy eflags to eax
        pushfd
        pop eax

        ; copy eflags to ecx to compare with later
        mov ecx, eax

        ; flip the id flag
        xor eax, 1 << 21

        ; copy eax to the flags
        push eax
        popfd

        ; copy the flags back to eax
        pushfd
        pop eax

        ; restore the flags to their original state
        push ecx
        popfd

        ; if eax and ecx arent equal then cpuid isnt supported
        xor eax, ecx
        jz .cpuid_no


        ; now we need to check if we support long mode
        ; first check if we have extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .long_no

        ; then check if we have long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29 ; test the long mode bit
        jz .long_no ; if the bit isnt set then we dont have long mode

        ; if we got this far we have long mode

        ; now we have set up paging
        ; i put this in a seperate C file because clang is better at this than i am
        call init_paging

        jmp $

        ; then tell the cpu where the table is
        ; mov eax, [tables.p4]
        ; mov cr3, eax

        ; enable physical address extension bit
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

        ; set long mode bit
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        ; enable paging
        ; TODO: this causes a 0xE (general protection fault) not sure why
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

        lgdt [descriptor64.ptr]
        call (descriptor64.code - descriptor64):kmain

    ; cpuid isnt avilable
    .cpuid_no:
        jmp $

    ; long mode isnt available
    .long_no:
        jmp $


    descriptor64:
        .null:
            dw 0xFFFF
            dw 0
            db 0
            db 0
            db 1
            db 0
        .code:
            dw 0
            dw 0 
            db 0
            db 10011010b
            db 10101111b
            db 0
        .data:
            dw 0
            dw 0
            db 0
            db 10010010b
            db 0
            db 0
        .end:
        .ptr:
            dw descriptor64 - descriptor64.end - 1
            dq descriptor64
