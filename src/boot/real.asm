extern SECTORS

extern start32

bits 16
section .real
    start16:
        cld
        cli
        clc

        ; zero out the segments
        xor ax, ax
        mov ds, ax
        mov fs, ax
        mov ss, ax
        
        not ax
        mov fs, ax

        mov sp, 0x7C00

        xor ax, ax

        ; check if we can read from the disk
        mov ah, 0x41
        mov bx, 0x55AA
        int 0x13
        
        jc fail_ext
        cmp bx, 0xAA55
        jnz fail_ext


        mov word [dap.sectors], SECTORS

        ; read from disk
        mov ah, 0x42
        mov si, dap
        int 0x13

        jc fail_disk

    enable_a20:
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

    prot:
        cli
        lgdt [descriptor]

        mov eax, cr0
        or eax, 1
        mov cr0, eax

        mov ax, descriptor.data - descriptor
        jmp (descriptor.code - descriptor):start32


    a20wait:
        in al, 0x64
        test al, 2
        jnz a20wait
        ret

    a20wait2:
        in al, 0x64
        test al, 1
        jz a20wait2
        ret


    a20check:
        mov ax, word [es:0x7DFE]
        cmp word [fs:0x7E0E], ax
        je .change
    .change:
        mov word [es:0x7DFE], 0x6969
        cmp word [fs:0x7E0E], 0x6969
        jne prot
    .end:
        mov word [es:0x7DFE], ax
        ret



    fail_ext:
        mov si, .msg
        jmp panic
        .msg: db "disk extensions not supported", 0

    fail_disk:
        mov si, .msg
        jmp panic
        .msg: db "failed to read from disk", 0

    fail_a20:
        mov si, .msg
        jmp panic
        .msg: db "failed to enable the a20 line", 0


    print:
        push ax
        cld
        mov ah, 0x0E
    .put:
        lodsb
        or al, al
        jz .end
        int 0x10
        jmp .put
    .end:
        pop ax
        ret


    panic:
        call print
    .end:
        cli
        hlt
        jmp .end

    dap:
        .len: db 0x10
        .zero: db 0
        .sectors: dw 0
        .addr: dd bootend
        .start: dq 1

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

        times 510 - ($-$$) db 0
        dw 0xAA55
    bootend: