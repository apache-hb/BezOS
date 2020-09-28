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
    xor si, si
    ; set continuation byte
    xor ebx, ebx
    ; put the memory map below the 480kb mark
    mov di, 0x7000
    mov es, di
    mov di, (0xFFFF - 2)

.next:
    inc si

    mov ecx, 24
    mov eax, 0xE820
    mov edx, 0x534D4150 ; 'SMAP'
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

    cli
    lgdt [gdt32]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp (gdt32.code - gdt32):start32


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

gdt32:
    dw gdt32.end - gdt32 - 1
    dd gdt32
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
bootmagic: dw 0xAA55
bootend:

bits 32
section .boot32
start32:
    mov ax, gdt32.data - gdt32
    mov ds, ax
    mov ss, ax

    ; wipe any existing characters on the vga text buffer
    mov ecx, 1000
.wipe:
    mov dword [0xB8000 + ecx * 4], 0
    dec ecx
    jnz .wipe
    mov dword [0xB8000], 0

    pushfd
    pop eax

    mov ecx, eax

    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    xor eax, ecx
    jz fail_cpuid

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb fail_cpuid_ext

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz fail_long

    jmp $

%macro error 2
fail_%1:
    mov edi, .msg
    jmp panic32
    .msg: db %2, 0
%endmacro

error cpuid, "cpuid instruction missing"
error cpuid_ext, "extended cpuid missing"
error long, "long mode not supported"

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

bits 64
section .boot64
    call kmain