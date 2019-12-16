MBALIGN equ 1 << 0
MEMINFO equ 1 << 1
FLAGS equ MBALIGN | MEMINFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

KERNEL_LMA equ 0x0000000000100000
KERNEL_VMA equ 0xFFFFFFFF80000000

%define ABS(x) ((x) - KERNEL_VMA)

section .boot
    bits 32
    align 4
    multiboot_header:
        dd MAGIC
        dd FLAGS
        dd CHECKSUM

    align 8
    global _boot:function
    _boot:
        cli
        mov esp, stack_front

        mov [mb_eax], eax
        mov [mb_ebx], ebx

        mov eax, cr0
        and eax, 0x7FFFFFFF
        mov cr0, eax

        mov edi, ABS(pml4)
        mov cr3, edi

        mov eax, cr4
        or eax, (1 << 5)
        mov cr4, eax

        mov ecx, 0xC0000080
        rdmsr
        or eax, (1 << 8)
        wrmsr

        mov eax, cr0
        or eax, (1 << 31)
        mov cr0, eax

        lgdt [gdt64_ptr]
        jmp boot64_low
    
    bits 64
    boot64_low:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        xor rax, rax
        mov ecx, 0xC0000100
        wrmsr
        mov ecx, 0xC0000101
        wrmsr

        jmp boot64_high

    die64_low:
        hlt
        jmp die64_low

    align 16
    gdt64:
        dq 0

        dw 0
        dw 0

        db 0
        db 0x98
        db 0xA0
        db 0

        dw 0
        dw 0

        db 0
        db 0x92
        db 0xC0
        db 0

    gdt64_ptr:
        dw 3 * 8 - 1
        dq gdt64

    mb_eax:
        dw 0
    mb_ebx:
        dw 0

section .init.text
    bits 64

    boot64_high:
        mov rsp, stack_front
        xor rbp, rbp

        xor rax, rax
        push rax
        popf

        mov eax, [mb_eax]
        mov edi, [mb_ebx]
        push 0

        extern kmain
        call kmain

    die64_high:
        hlt
        jmp die64_high

section .init.data
    align 0x1000
    pml4:
        dq ABS(pdp0) + 7
        times 255 * 8 db 0
        dq ABS(pdp0) + 7
        times 255 * 8 db 0
        dq ABS(pdp1) + 7

    align 0x1000
    pdp0:
        dq ABS(pd + 0x0000) + 7
        dq ABS(pd + 0x1000) + 7
        dq ABS(pd + 0x2000) + 7
        dq ABS(pd + 0x3000) + 7

    align 0x1000
    pdp1:        
        times 510 * 8 db 0
        dq ABS(pd + 0x0000) + 7
        dq ABS(pd + 0x1000) + 7

    align 0x1000
    pd:
    %assign index 0
    %rep 512 * 4
        dq (index << 21) + 0x87
    %assign index index + 1
    %endrep

    ; reserve 16K for a stack
    align 0x1000
    stack_back:
        times 1024 * 16 db 0
    stack_front:
