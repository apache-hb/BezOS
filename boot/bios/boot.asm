extern SECTORS
extern kmain

bits 16
section .boot16
prelude:
    cli
    cld
    jmp 0:entry
entry:
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7C00

    ; set fs to max to test for a20 wraparound later
    not ax
    mov fs, ax

    ; check for lba disk extensions
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    cmp bx, 0xAA55

    jne fail_ext
    jc fail_ext

    jmp $

%macro fail 2
fail_%1:
    mov si, .msg
    jmp panic
    .msg: db %2, 0
%endmacro

fail ext, "missing lba extension"
fail disk, "failed to read from disk"
fail a20, "failed to disable the a20 line"
panic:
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

dap:
    .size: db 0x10
    .zero: db 0
    .sectors: dw SECTORS
    .addr: dd bootend
    .start: dq 1

    times 510 - ($-$$) db 0
bootmagic: dw 0xAA55
bootend:

bits 32
section .boot32

bits 64
section .boot64
    call kmain