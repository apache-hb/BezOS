extern start64

global start32

global GDT64


%define VGA_HEIGHT 25
%define VGA_WIDTH 80
%define VGA_BUFFER 0xB8000


bits 32
section .prot
    start32:
        mov ds, ax
        mov ss, ax
        
        xor ax, ax

        mov gs, ax
        mov fs, ax
        mov es, ax

        call clear


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
        jz fail_cpuid

        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb fail_cpuid

        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz fail_long


        mov eax, cr0
        and eax, (-1) - ((1 << 2) | (1 << 3))
        mov cr0, eax

        fninit
        fnstsw [fpudata]
        cmp word [fpudata], 0
        jne fpudata

        mov eax, 1
        cpuid
        test edx, 1 << 25
        jz fail_sse

        mov eax, cr0
        and eax, ~(1 << 2)
        or eax, 1
        mov cr0, eax

        mov eax, cr4
        or eax, (1 << 9) | (1 << 10)
        mov cr4, eax

        mov eax, cr4
        or eax, (1 << 5) | (1 << 4)
        mov cr4, eax

        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        call paging

        mov cr3, eax

        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

        lgdt [GDT32]

        mov ax, descriptor64.kdata
        jmp descriptor64.kcode:start64


    paging:

    clear:
        mov ecx, 0xB8000
        mov eax, -4000
    .next:
        mov word [ecx + eax + 4000], 8192
        add eax, 2
        jne .next
        ret


    print:
        cld
        xor eax, eax
        mov ebx, 0xB8000
    .put:
        lodsb
        or al, al
        jz .end
        mov ah, 7
        mov word [ebx], ax
        add ebx, 2
        jmp .put
    .end:
        ret


    panic:
        call print
    .end:
        cli
        hlt
        jmp .end


    fail_cpuid:
        mov eax, .msg
        jmp panic
        .msg: db "cpuid not detected", 0

    fail_fpu:
        mov eax, .msg
        jmp panic
        .msg: db "fpu unit not detected", 0

    fail_sse:
        mov eax, .msg
        jmp panic
        .msg: db "see is not detected", 0

    fail_long:
        mov eax, .msg
        jmp panic
        .msg: db "64 bit not detected", 0


    fpudata: dw 0xFFFF

    align 16
    GDT32:
        dw descriptor64.end - descriptor64 - 1
        dd descriptor64

    align 16
    GDT64:
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
        .kcode: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 10011010b
            db 00100000b
            db 0
        .kdata: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 10010010b
            db 00000000b
            db 0
        .ucode: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 11011010b
            db 00000000b
            db 0
        .udata: equ $ - descriptor64
            dw 0
            dw 0
            db 0
            db 11010010b
            db 00000000b
            db 0
        .end: