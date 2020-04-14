extern kmain

extern BOOT_SECTORS

extern BOOT_END

struc partition
    .attrib: resb 1
    .start: resb 3
    .type: resb 1
    .end: resb 3
    .lba: resd 1
    .sectors: resd 1
endstruc

bits 16
section .real
    init:
        cli
        jmp 0:start16
    start16:
        xor ax, ax
        mov ds, ax
        mov ss, ax
        mov sp, 0x7C00

        ; setup these segments so we can check if the a20 line is enabled later on
        mov es, ax
        not ax
        mov fs, ax
        sti

        clc

        mov ah, 0x41
        mov bx, 0x55AA
        int 0x13

        jc fail_ext
        cmp bx, 0xAA55
        jc fail_ext

        mov word [dap.sectors], BOOT_SECTORS
        mov ah, 0x42
        mov si, dap
        int 0x13

        jc fail_disk

    a20enable:
        call a20check

    .bios:
        mov ax, 0x2401
        int 0x15

        call a20check

    .kbd:
        cli
        
        call a20wait
        mov al, 0xAD
        out 0x64, al

        call a20wait
        mov al, 0xD0
        out 0x64, al

        call a20wait2
        in al, 0x60
        push eax

        call a20wait
        mov al, 0xD1
        out 0x64, al

        call a20wait
        pop eax
        or al, 2
        out 0x64, al

        call a20wait
        mov al, 0xAE
        out 0x64, al

        call a20wait
        sti

        call a20check
    .port:
        in al, 0xEE
        call a20check
    .fail:
        jmp fail_a20


    a20check:
        mov ax, word [es:0x7DFE]
        cmp word [fs:0x7E0E], ax
        je .change
    .change:
        mov word [es:0x7DFE], 0x6969
        cmp word [fs:0x7E0E], 0x6969
        jne e820map
    .end:
        mov word [es:0x7DFE], ax
        ret
        
    a20wait:
        in al, 0x64
        test al, 2
        jnz a20wait
        ret

    a20wait2:
        in al, 0x65
        test al, 1
        jz a20wait2
        ret



    fail_ext:
        mov si, .msg
        jmp panic
        .msg: db "disk extensions not support", 0

    fail_disk:
        mov si, .msg
        jmp panic
        .msg: db "failed to read from disk", 0

    fail_a20:
        mov si, .msg
        jmp panic
        .msg: db "failed to enable a20 line", 0

    fail_e820:
        mov si, .msg
        jmp panic
        .msg: db "failed to read e820 map", 0

    panic:
        cld
        mov ah, 0x0E
    .put:
        lodsb
        or al, al
        jz .end
        int 0x10
        jmp .put
    .end:
        cli
        hlt
        jmp .end

    align 4
    dap:
        .len: db 0x10
        .zero: db 0
        .sectors: dw 0
        .offset: dw signature + 2
        .segment: dw 0
        .addr: dq 1

    align 4
    descriptor:
        .null:
            dw descriptor.end - descriptor - 1
            dd descriptor
            dw 0
        .code:
            dw 0xFFFF
            dw 0
            db 0
            db 0x9A
            db 11001111b
            db 0
        .data:
            dw 0xFFFF
            dw 0
            db 0
            db 0x92
            db 11001111b
            db 0
        .end:

    times 440 - ($-$$) db 0
    mbr:
        .udid: dd 0
        .zero: dw 0
        .pte0: 
            istruc partition 
            iend
        .pte1:
            istruc partition 
            iend
        .pte2:
            istruc partition 
            iend
        .pte3:
            istruc partition 
            iend
    signature: dw 0xAA55

    e820_ptr: dd 0

    e820map:
        mov edi, BOOT_END
        xor ebx, ebx

        jmp .begin

    .next:
        add edi, 24
    .begin:
        mov eax, 0xE820
        mov edx, 0x534D4150
        mov ecx, 24

        int 0x15

        ; make sure the e820 call was valid
        cmp eax, 0x534D4150
        jne fail_e820

        cmp ecx, 20
        je .fix_size

        jmp .next

    .fix_size:
        mov dword [edi+20], 0
        cmp ebx, 0
        jne .next
    .end:
        mov dword [e820_ptr], edi
        jmp enter_prot

    enter_prot:
        cli

        lgdt [descriptor]

        mov eax, cr0
        or eax, 1
        mov cr0, eax

        mov ax, (descriptor.data - descriptor)
        jmp (descriptor.code - descriptor):start32

bits 32
section .prot
    start32:
        jmp $

bits 64
section .long
    start64:
        mov rax, kmain
        call rax