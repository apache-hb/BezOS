; real mode assembly

extern start32

section .bootloader
; we're only 16 bits right now
bits 16
    start:
        mov ah, 0 ; clear disk status
        int 0x13 ; int 0x13, ah = 0

        ; read in 32 bit code from disk and put it in the sector afterwards
        mov bx, 0x7E00 ; read it in after this memory 
        mov al, 1 ; read 1 sector (512 bytes)
        mov ch, 0 ; from the first cylinder
        mov dh, 0 ; and the first head
        mov cl, 2 ; the second sector (we are in the first sector already)
        mov ah, 2 ; ah = 2 means read from disk
        int 0x13 ; disk interrupt

        ; prepare to go to protected mode
        cli ; we wont be needing interrupts for now

        ; enable the a20 line

        mov ax, 0x2403 ; does this cpu support the a20 gate?
        int 0x15
        jb .a20_no ; no support
        cmp ah, 0
        jb .a20_no ; no support

        mov ax, 0x2402 ; what is the status of the a20 gate?
        int 0x15
        jb .a20_fail ; failed to get status
        cmp ah, 0
        jnz .a20_fail ; failed to get status

        cmp al, 1
        jz .a20_on ; the a20 gate is already activated

        mov ax, 0x2041 ; lets try to activate the a20 gate
        int 0x15
        jb .a20_fail ; failed to activate the gate
        cmp ah, 0
        jnz .a20_fail ; failed to activate the gate

    .a20_on: ; the a20 works
        ; load the global descriptor table
        lgdt [descriptor]

        ; go to protected mode
        mov eax, cr0
        or eax, 1 ; set the protected mode bit
        mov cr0, eax


        ; jump to 32 bit code
        jmp (descriptor.code - descriptor):start32

    .a20_fail: ; failed to activate the a20 gate
        mov si, fail_a20
        jmp real_panic
    .a20_no: ; the a20 gate isnt supported
        mov si, no_a20
        ; fallthrough to real_panic

    ; panic in real mode
    ; never returns
    ; set si = message
    real_panic:
        cld
        mov ah, 0x0E
    .put:
        lodsb ; put next character in al
        or al, al ; is character null
        jz end ; if it is then halt
        int 0x10 ; write char intterupt
        jmp .put ; write again
    end:
        cli
        hlt
        jmp end

    fail_a20 db "a20 line failed", 0
    no_a20 db "a20 line not supported", 0

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

    ; pad the rest of the file with 0x0 up to 510
    times 510-($-$$) db 0
    db 0x55 ; file[511] = 0x55
    db 0xAA ; file[512] = 0xAA
    ; boot magic number to mark this as a valid bootsector