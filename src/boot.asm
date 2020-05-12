extern SECTORS

bits 16
section .real
prelude:
    cli
    cld
    jmp 0x0:start16
start16:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ss, ax
    
    not ax
    mov fs, ax
    
    mov sp, 0x7C00

    sti

    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13

    cmp bx, 0xAA55
    jc fail_ext    
    jnz fail_ext

    push ds
    pop word [dap.segment]
    mov word [dap.sectors], SECTORS


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

    jmp fail_a20

preload32:
    cli
    lgdt [desc16]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    mov ax, desc16.data - desc16
    mov bx, desc16.code - desc16
    jmp (desc16.code - desc16):start32


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
    jne preload32
.end:
    mov word [es:0x7DFE], ax
    ret

align 4
dap:
    .len: db 16
    .zero: db 0
    .sectors: dw 0
    .offset: dw bootend
    .segment: dw 0
    .start: dq 1

fail_ext:
    mov si, .msg
    jmp panic16
    .msg: db "disk extensions not supported", 0

fail_disk:
    mov si, .msg
    jmp panic16
    .msg: db "failed to read from disk", 0

fail_a20:
    mov si, .msg
    jmp panic16
    .msg: db "failed to enable the a20 line", 0

print16:
    cld
    mov ah, 0x0E
.put:
    lodsb
    or al, al
    jz .end
    int 0x10
    jmp .put
.end:
    ret

; si = msg
panic16:
    call print16
.end:
    cli
    hlt
    jmp .end


desc16:
    .null:
        dw desc16.end - desc16 - 1
        dd desc16
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

    times 0x1B4 - ($-$$) db 0
    mbr: times 10 db 0
    pt1: times 16 db 0
    pt2: times 16 db 0
    pt3: times 16 db 0
    pt4: times 16 db 0
signature:
    dw 0xAA55
bootend:


bits 32
section .prot
start32:
    jmp $

bits 64
section .long
start64:
    nop
