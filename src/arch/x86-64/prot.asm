; 64 bit kernel main function
extern kmain

section .protected
bits 32
    global start32:function
    start32:
        dw 0xFFFF
        pop ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov fs, ax
        mov gs, ax
        
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
        ; call setup_paging

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
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

        lgdt [descriptor64.ptr]
        call (descriptor64.code - descriptor64):kmain

    ; cpuid isnt avilable
    .cpuid_no:
        mov eax, fail_cpuid
        jmp prot_panic

    ; long mode isnt available
    .long_no:
        mov eax, fail_long
        jmp prot_panic

    ; eax = error string
    prot_panic:
        cld
        mov ebx, 0x8B00
        mov ch, 11110000b
    .put:
        mov cl, [eax]
        or cl, cl
        jz .end
        mov word [ebx], cx
        add ebx, 2
        inc eax
        jmp .put
    .end:
        cli
        hlt
        jmp .end

    fail_cpuid db "cpuid not available", 0
    fail_long db "long mode not available", 0

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
