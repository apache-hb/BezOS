extern BOOTSECTORS
extern BOOTEND
extern kmain

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
    mov word [dap.sectors], BOOTSECTORS

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

load_e820:
    mov qword [bootinfo.mem], BOOTEND
    mov eax, 0xE820 ; function
    xor ebx, ebx ; continuation, must be 0 for first call
    mov di, [bootinfo.mem] ; buffer addr
    mov ecx, 24 ; buffer size
    mov edx, 'SMAP' ; signature

.next:
    int 15

    ; check the continuation byte
    or ebx, ebx
    jnz .next
    inc dword [bootinfo.num]

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
    jne load_e820
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

    ; check if we have cpuid
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

    ; check for extended cpuid
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb fail_cpuid

    ; check for long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz fail_long

    ; enable the fpu and sse
    mov eax, cr0
    and eax, (-1) - ((1 << 2) | (1 << 3))
    mov cr0, eax

    ; if the store fails then there is no fpu
    fninit
    fnstsw [fpu_data]
    cmp word [fpu_data], 0
    jne fail_fpu

    ; enable sse and sse2
    mov eax, 1
    cpuid
    test edx, 1 << 25
    jz fail_sse

    ; disable emulation and enable the coproccessor
    mov eax, cr0
    and eax, ~(1 << 2)
    or eax, 1
    mov cr0, eax

    ; enabled fpu save/store and sse exceptions
    mov eax, cr4
    or eax, (1 << 9) | (1 << 10)
    mov cr4, eax

    ; time to enable paging
    ; TODO: setup tables

    jmp $

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; load the descriptor table and jump to 64 bit code
    lgdt [low_gdt]

    jmp descriptor64.code:start64

align 16
low_gdt:
    dw descriptor64.end - descriptor64 - 1
    dd descriptor64

align 16
high_gdt:
    dw descriptor64.end - descriptor64 - 1
    dq descriptor64

align 16
descriptor64:
    .null:
        dw 0
        dw 0
        db 0
        db 0
        db 0
        db 0
    .code: equ $ - descriptor64
        dw 0
        dw 0
        db 0
        db 10011010b
        db 00100000b
        db 0
    .data: equ $ - descriptor64
        dw 0
        dw 0
        db 0
        db 10010010b
        db 0
        db 0
    .end:

%macro error 2 
fail_%1:
    mov edi, .msg
    jmp panic32
    .msg: db %2, 0
%endmacro

fpu_data: dw 0xFFFF

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

bootinfo:
    ; pointer to the memory map
    .mem: dq 0
    ; number of entries in the memory map
    .num: dd 0

align 4
bits 64
section .boot64
start64:
    mov ax, descriptor64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    lgdt [high_gdt]
    jmp enter_long
enter_long:
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    mov rbp, rsp
    movsx rsp, sp

    push bootinfo
    call kmain