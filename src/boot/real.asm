extern SECTORS

extern start32

bits 16
section .real
    start16:
        cli
        cld
        jmp 0:init
    init:

        ; zero out the segments
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        mov sp, 0x7C00

        sti

        ; check if we can read from the disk
        mov ah, 0x41
        mov bx, 0x55AA
        int 0x13
        
        jc fail_ext
        cmp bx, 0xAA55
        jnz fail_ext

        push ds
        pop word [dap.segment]

        mov word [dap.sectors], SECTORS

        ; read from disk
        mov ah, 0x42
        mov si, dap
        int 0x13

        jc fail_disk


        ; enable the a20 line to disable wraparound
    enable_a20:
        ; if the a20 line has been enabled by the bios 
        ; then we can just skip ahead
        call a20check
    .bios:
        ; first we try the bios interrupt
        mov ax, 0x2401
        int 0x15

        call a20check
    .kbd:
        ; next we try the keyboard controller
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
        ; then finally we try the magic port
        in al, 0xEE
        call a20check
    .fail:
        ; if the a20 line still isnt enabled then just give up
        jmp fail_a20

        ; next lets load the global descriptor table
    prot:
        cli
        lgdt [descriptor]

        ; then enable protected mode
        mov eax, cr0
        or eax, 1
        mov cr0, eax

        ; and then jump to protected mode code
        mov ax, descriptor.data - descriptor
        mov bx, descriptor.code - descriptor
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

    align 4
    dap:
        .len: db 16
        .zero: db 0
        .sectors: dw 0
        .offset: dw signature + 2
        .segment: dw 0
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

    struc partition
        .attrib: resb 1
        .start: resb 3
        .type: resb 1
        .end: resb 3
        .lba: resd 1
        .sectors: resd 1
    endstruc

    times (0x1B4 - ($-$$)) db 0
    mbr: times 10 db 0
    pt1: times 16 db 0
    pt2: times 16 db 0
    pt3: times 16 db 0
    pt4: times 16 db 0
    
    signature: dw 0xAA55
    