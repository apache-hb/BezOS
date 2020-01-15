extern start64

extern print
extern init_paging
extern init_vga

section .protected
bits 32
    global start32
    start32:
        ; when we get here the bootloader has set 
        ; ax = data segment

        ; so now we set the segments
        mov ds, ax
        mov ss, ax

        ; clear general segments
        xor ax, ax

        mov gs, ax
        mov fs, ax
        mov es, ax

        ; enable printing to vga for logging
        call init_vga

    enable_cpuid:
        ; check if we have cpuid by checking eflags
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
        jz cpuid_fail

        ; if we have cpuid we need extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb cpuid_fail

        ; now we use cpuid to check for long mode
        mov eax, 0x80000001
        cpuid 
        test edx, 1 << 29
        jz long_fail


    enable_features:
        ; enable some cpu features for later use
        ; first enable the fpu because floats seem useful
        mov eax, cr0
        and eax, (-1) - ((1 << 2) | (1 << 3))
        mov cr0, eax

        fninit
        fnstsw [fpu_data]
        cmp word [fpu_data], 0
        jne fpu_fail ; if the fpu fails to store data then we dont have an fpu unit

        ; next enable sse and sse2
        mov eax, 1
        cpuid
        test edx, 1 << 25
        jz sse_fail

        mov eax, cr0
        ; disable emulation
        and eax, ~(1 << 2)
        ; enable coproccessor
        or eax, 1
        mov cr0, eax

        ; enable fpu save/restore functions and sse exceptions
        mov eax, cr4
        or eax, (1 << 9) | (1 << 10)
        mov cr4, eax

        ; time to enable paging
        call init_paging

        ; move the top page table to cr3
        mov cr3, eax


        ; time to enable PAE for 64 bit addresses in the page table
        ; this is also required for long mode

        mov eax, cr4
        or eax, (1 << 5)
        mov cr4, eax

        ; enable paging and WP flag for read only memory support
        mov eax, cr0
        or eax, 0x80010001
        mov cr0, eax


        ; setup the gdt

        ; jump to 64 bit main
        
        ; enable long mode
        mov eax, 0xC0000080
        rdmsr
        or ebx, 1 << 8
        wrmsr

        ; load the 64 bit gdt to enable full 64 bit mode
        lgdt [descriptor64.ptr]

        ; then long jump to main
        jmp descriptor64.code:start64


    cpuid_fail:
        push cpuid_msg
        jmp panic

    long_fail:
        push long_msg
        jmp panic

    fpu_fail:
        push fpu_msg
        jmp panic

    sse_fail:
        push sse_msg
        jmp panic

    panic:
        call print
    .end:
        cli
        hlt
        jmp .end


    cpuid_msg: db "cpuid not detected", 0
    long_msg: db "cpu is 32 bit, 64 bit is required", 0
    fpu_msg: db "fpu unit is not detected", 0
    sse_msg: db "sse is not detected", 0

    fpu_data: dw 0xFFFF

    global descriptor64.data
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
            db 0
            db 10011010b
            db 10101111b
            db 0
        .data:
            dw 0
            dw 0
            db 0
            db 0
            db 10010010b
            db 00000000b
            db 0
        .ptr:
            dw $ - descriptor64 - 1
            dq descriptor64
        .end:

    ;     ; we need to do segmentation because protected mode needs it
    ;     mov ds, ax
    ;     mov es, ax
    ;     mov fs, ax
    ;     mov ss, ax
    ;     mov gs, ax

    ;     ; check eflags to see if we have cpuid
    ;     pushfd
    ;     pop eax

    ;     mov ecx, eax

    ;     xor eax, 1 << 21

    ;     push eax
    ;     popfd

    ;     pushfd
    ;     pop eax

    ;     push ecx
    ;     popfd

    ;     xor eax, ecx
    ;     jz .no_cpuid

    ;     ; check if we have extended cpuid
    ;     mov eax, 0x80000000
    ;     cpuid
    ;     cmp eax, 0x80000001
    ;     jb .no_long

    ;     ; check if we have long mode
    ;     mov eax, 0x80000001
    ;     cpuid
    ;     test edx, 1 << 29
    ;     jz .no_long

    ;     ; lets enable floating point numbers as well
    ;     ; those seem important
    ;     mov eax, cr0
    ;     and eax, (-1) - ((1 << 2) | (1 << 3))
    ;     mov cr0, eax

    ;     ; enable the fpu unit
    ;     fninit
    ;     fnstsw [fpu_test]
    ;     cmp word [fpu_test], 0 ; if nothing is written then we dont have an fpu unit
    ;     jne .no_fpu

    ;     ; lets enable sse and sse2 now
    ;     mov eax, 0x1
    ;     cpuid 
    ;     test edx, 1 << 25
    ;     jz .no_sse

    ;     mov eax, cr0
    ;     ; disable emulation bit
    ;     and eax, ~(1 << 2)
    ;     ; enable co proccessor bit
    ;     or eax, (1 << 1)
    ;     mov cr0, eax

    ;     ; enable the fxsave/restore functions and sse exceptions
    ;     mov eax, cr4
    ;     or eax, (1 << 9) | (1 << 10)
    ;     mov cr4, eax

    ;     ; now we setup paging
    ;     ; eax = pointer to top level page
    ;     call setup_paging

    ;     ; if pml5 is supported then enable it
    ;     mov bl, byte [MEMORY_LEVEL]
    ;     cmp bl, 5
    ;     jne .pml4_only

    ;     ; enable 5 level paging
    ;     mov edx, cr4
    ;     or edx, 1 << 12
    ;     mov cr4, edx

    ; .pml4_only:

    ;     ; enable PAE
    ;     mov ecx, cr4
    ;     or ecx, 1 << 5
    ;     mov cr4, ecx

    ;     ; enable the long mode bit
    ;     mov ebx, 0xC0000080
    ;     rdmsr
    ;     or ebx, 1 << 8
    ;     wrmsr

    ;     ; cr3 stores the top level page table
    ;     mov eax, dword [eax]
    ;     mov cr3, eax

    ;     ; enable paging
    ;     mov ecx, cr0
    ;     or ecx, 1 << 31
    ;     mov cr0, ecx

    ;     jmp $

    ;     call setup_gdt

    ;     jmp 0x10:kmain

    ; ; if we are here we dont have cpuid
    ; .no_cpuid:
    ;     mov eax, cpuid_msg
    ;     jmp prot_panic

    ; ; if we are here we dont have extended cpuid and therefore no long mode
    ; .no_extended_cpuid:
    ;     mov eax, ext_cpuid_msg
    ;     jmp prot_panic

    ; ; if we got here we dont have long mode
    ; .no_long:
    ;     mov eax, long_msg
    ;     jmp prot_panic

    ; ; if we get here we have no fpu unit (but protected mode)
    ; ; this is either one funky cpu or its buggy
    ; .no_fpu:
    ;     mov eax, fpu_msg
    ;     jmp prot_panic

    ; ; if we get here we dont support sse
    ; .no_sse:
    ;     mov eax, sse_msg
    ;     jmp prot_panic

    ; ; panic by printing an error then hanging
    ; ; eax = pointer to error message
    ; prot_panic:
    ;     push eax
    ;     call prot_print
    ; .end:
    ;     cli
    ;     hlt
    ;     jmp .end

    ; sse_msg: db "this cpu doesnt support sse", 0
    ; fpu_msg: db "this cpu doesnt have an fpu unit", 0
    ; long_msg: db "this cpu doesnt support long mode", 0
    ; cpuid_msg: db "this cpu doesnt support cpuid", 0
    ; ext_cpuid_msg: db "this cpu doesnt support extended cpuid", 0

    ; ; data to test the fpu with
    ; fpu_test: dw 0xFFFF