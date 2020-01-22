extern kmain

extern high_gdt

section .long
bits 64
    global start64
    start64:
        ; ax is set to the data segment here
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        lgdt [high_gdt]

        call setup_long
    
    setup_long:

        xor rax, rax
        xor rbx, rbx
        xor rcx, rcx
        xor rdx, rdx
        movsx rsp, sp
        mov rbp, rsp

        call kmain