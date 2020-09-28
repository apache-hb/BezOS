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

    mov ah, 0x42
    mov si, dap
    int 0x13

    call a20check

    mov ax, 0x2401
    int 0x15
    call a20check

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
    out 0x60, al

    call a20wait
    mov al, 0xAE
    out 0x64, al

    call a20wait
    sti

    call a20check

    in al, 0x92
    or al, 2
    out 0x92, al
    call a20check
    
    jmp fail_a20

load_e820:
    clc
    mov si, 0
    ; set continuation byte
    mov ebx, 0
    ; put the memory map below the 480kb mark
    mov di, 0x7000
    mov es, di
    mov di, (0xFFFF - 2)

.next:
    inc si
    mov ecx, 24
    mov eax, 0xE820
    mov edx, 'SMAP'
    sub di, 24

    int 0x15
    jc fail_e820

    cmp ecx, 20
    jne .skip
    mov dword [es:di + 20], 0
.skip:

    cmp ebx, 0
    jne .next

    mov [es:0xFFFF - 2], si

    jmp $


a20wait:
    in al, 0x64
    test al, 2
    jnz a20wait
    ret

a20wait2:
    in al, 0x64
    test al, 2
    jz a20wait2
    ret

a20check:
    ; compare the bootmagic using wraparound
    mov word [ds:0x7DFE], 0x6969
    cmp word [fs:0x7E0E], 0x6969

    ; if there is no wraparound then the a20 line is enabled
    jne load_e820
    ret

%macro fail 2
fail_%1:
    mov si, .msg
    jmp panic
    .msg: db %2, 0
%endmacro

fail ext, "missing lba extension"
fail disk, "failed to read from disk"
fail a20, "failed to enable the a20 line"
fail e820, "failed to collect memory map"
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