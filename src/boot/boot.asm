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
    mov word [dap.sectors], SECTORS

    ; read from the disk
    mov ah, 0x42
    mov si, dap
    int 0x13

    jc fail_disk

    ; time to enable the a20 line

    ; if its already enabled then just skip it all
    call a20check

    ; try the bios call
    mov ax, 0x2401
    int 0x15
    call a20check

    ; now try the keyboard method

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

    ; check again
    call a20check

    ; here we try the fast a20 method
    in al, 0x92
    or al, 2
    out 0x92, al
    call a20check

    ; if none of these methods work then fail
    jmp fail_a20

    ; time to load the gdt for protected mode
load_gdt:
    cli
    lgdt [descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; jump to protected mode code
    jmp (descriptor.code - descriptor):start32

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
    mov word [es:0x7DFE], 0x6969
    cmp word [fs:0x7E0E], 0x6969

    ; if there is no wraparound then the a20 line is enabled
    jne load_gdt
    ret

%macro fail 2
fail_%1:
    mov si, .msg
    jmp panic
    .msg: db %2, 0
%endmacro

fail ext, "no extension"
fail disk, "bad disk"
fail a20, "bad a20"

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


dap:
    .size: db 0x10 ; size of the packet
    .zero: db 0 ; always zero
    .sectors: dw 0 ; number of sectors to read
    .addr: dd bootend
    .start: dq 1 

    times 510 - ($-$$) db 0
bootmagic:
    dw 0xAA55
bootend:


bits 32
section .boot32
start32:
    mov ax, (descriptor.data - descriptor)
    mov ds, ax
    mov ss, ax
    jmp fail_cpuid

%macro error 2 
fail_%1:
    mov edi, .msg
    jmp panic32
    .msg: db %2, 0
%endmacro

error cpuid, "no cpuid"
error long, "no long mode"
error fpu, "no fpu"
error sse, "no sse"

; edi = message
panic32:
    mov ebx, 0xB8000
    mov ah, 7
.put:
    mov al, [edi]
    or al, al
    jz .end
    mov word [ebx], ax
    add ebx, 2
    inc edi
    jmp .put
.end:
    cli
    hlt
    jmp .end

align 4
bits 64
section .boot64
start64:
    jmp $