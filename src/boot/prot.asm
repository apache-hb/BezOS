extern start64

extern BSS_SIZE
extern BSS_ADDR

global start32

global GDT64

bits 32
section .prot
    start32:
        mov ds, ax
        mov ss, ax

        xor ax, ax

        mov gs, ax
        mov fs, ax
        mov es, ax

        mov esp, 0x7C00

        call clear

        ; check if we have cpuid
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


        ; if we dont have cpuid then we cant have long mode
        ; we fail in this case
        xor eax, ecx
        jz fail_cpuid

        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb fail_cpuid

        ; now use cpuid to check if we have long mode
        ; if we dont then fail
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz fail_long

        ; initialize the float point unit and make sure it works
        fninit
        fnstsw [fpudata]
        cmp word [fpudata], 0
        jne fail_fpu

        ; make sure we have sse as well
        mov eax, 1
        cpuid
        test edx, 1 << 25
        jz fail_sse

        ; tell the cpu that an fpu is present (what?)
        mov eax, cr0
        and eax, ~(1 << 2)
        mov cr0, eax

        ; enable PAE, and some sse stuff
        mov eax, cr4
        or eax, (1 << 9) | (1 << 10) | (1 << 5) | (1 << 7)
        mov cr4, eax

        ; setup basic paging
        ; we will replace the page tables in long mode in the kernel
        call paging

        ; enable paging
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

        ; enable long mode
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        ; load the 32 bit descriptor ptr
        lgdt [GDT32]

        ; jump to the kernel
        mov ax, descriptor64.kdata
        jmp descriptor64.kcode:start64


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

    ;                                  _           _
    ;   __ _  ___ _ __   ___ _ __ __ _| |_ ___  __| |
    ;  / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \/ _` |
    ; | (_| |  __/ | | |  __/ | | (_| | ||  __/ (_| |
    ;  \__, |\___|_| |_|\___|_|  \__,_|\__\___|\__,_|
    ;  |___/

    ; clear the vga monitor
    clear:
        mov eax, -4000
    .next:
        mov word [eax + 757664], 0
        add eax, 2
        jne .next

        ; clear the bss section
        mov eax, BSS_SIZE
        mov ecx, BSS_ADDR
        lea eax, [eax + BSS_ADDR]
        cmp ecx, eax
    .zero:
        mov dword [4 * ecx + BSS_ADDR], 0
        inc ecx
        cmp ecx, eax
        jl .zero

        ret 

    ; setup paging and identity map the first mb of memory
    paging:
        mov eax, pd
        mov ecx, pt
        mov dword [pdpt+4], 0
        mov dword [pd+4], 0
        or eax, 1
        or ecx, 1
        mov dword [pdpt], eax
        mov dword [pd], ecx
        mov eax, pt
        mov ecx, 3
    .next:
        mov dword [eax], ecx
        add ecx, 0x1000
        mov dword [eax + 4], 0
        add eax, 8
        cmp ecx, 0x100003
        jne .next
        mov eax, pdpt
        mov cr3, eax


        mov     word [753664], 1896
        mov     word [753666], 1893
        mov     word [753668], 1900
        mov     word [753670], 1900
        mov     word [753672], 1903
        mov     word [753674], 1792

        ret

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
        .kcode:
            dw 0
            dw 0
            db 0
            db 10011010b
            db 00100000b
            db 0
        .kdata:
            dw 0
            dw 0
            db 0
            db 10010010b
            db 00000000b
            db 0
        .ucode:
            dw 0
            dw 0
            db 0
            db 11011010b
            db 00000000b
            db 0
        .udata:
            dw 0
            dw 0
            db 0
            db 11010010b
            db 00000000b
            db 0
        .end:

section .bss
    align 0x1000
    pd: resb 0x1000

    align 0x1000
    pt: resb 0x1000

    align 0x20
    pdpt: resb 32
