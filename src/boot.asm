extern SECTORS


bits 16
section .boot16
prelude:
    cli
    cld
    jmp 0:start16
start16:
    xor ax, ax

    ; zero the data segment to make addressing easy
    mov ds, ax

    ; zero the stack segment and load the stack before the bootloader
    mov ss, ax
    mov sp, 0x7C00

    ; here the fs is set to 0xFFFF and es is set to 0
    ; this lets us test a20 wraparound later in the bootloader
    ; this is because [0xFFFF:0] and [0:0] point to the same address
    ; when wraparound is enabled
    mov es, ax
    not ax
    mov fs, ax

    ; make sure the disk extensions are supported
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    cmp bx, 0xAA55

    ; if the extensions arent supported then fail
    jnz fail_ext
    jc fail_ext

    ; load the number of required sectors
    mov dword [dap.sectors], SECTORS

    ; read from the disk
    mov ah, 0x42
    mov si, dap
    int 0x13

    jc fail_disk

    ; time to enable the a20 line

    jmp fail_disk
    jmp $
    
fail_ext:
    mov si, .msg
    jmp panic
    .msg: db "no extension", 0

fail_disk:
    mov si, .msg
    jmp panic
    .msg: db "bad disk", 0

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
    .sectors: dw 0
    .bootend: dd bootend
    .start: dq 1

    times 510 - ($-$$) db 0
bootmagic:
    dw 0xAA55
bootend:


bits 32
section .boot32
start32:
    jmp $


bits 64
section .boot64
start64:
    jmp $