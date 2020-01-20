global start32

extern start64

extern vga_print
extern vga_init
extern init32

section .prot
bits 32
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
        call vga_init

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
        call init32

        ; move the top page table to cr3
        mov cr3, eax

        ; time to enable PAE for 64 bit addresses in the page table
        ; this is also required for long mode

        mov eax, cr4
        or eax, (1 << 5) | (1 << 4)
        mov cr4, eax

        ; enable long mode
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        ; enable paging and WP flag for read only memory support
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

        ; load the 64 bit gdt to enable full 64 bit mode
        lgdt [low_gdt]

        mov ax, descriptor64.data

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
        call vga_print
    .end:
        cli
        hlt
        jmp .end


    cpuid_msg: db "cpuid not detected", 0
    long_msg: db "cpu is 32 bit, 64 bit is required", 0
    fpu_msg: db "fpu unit is not detected", 0
    sse_msg: db "sse is not detected", 0

    fpu_data: dw 0xFFFF

    global high_gdt

    align 16
    low_gdt:
        dw descriptor64.end - descriptor64 - 1
        dd descriptor64

    align 16
    high_gdt:
        dw descriptor64.end - descriptor64 - 1
        dq descriptor64

    align 16
    descriptor64:
        .null:
            dw 0
            dw 0
            db 0
            db 0
            db 0
            db 0
        .code: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 10011010b
            db 00100000b
            db 0
        .data: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 10010010b
            db 00000000b
            db 0
        .end:
