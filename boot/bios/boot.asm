extern BOOT_SECTORS
extern SECTORS
extern PT_ADDR
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

    ; read bootstages in from disk
    mov ah, 0x42
    mov si, dap
    int 0x13

    jc fail_disk

    ; read rest of kernel in at the 1MB mark
    mov dword [dap.addr], 0x100000
    mov word [dap.sectors], SECTORS
    mov dword [dap.start], BOOT_SECTORS
    int 0x13

    jc fail_disk

    call a20check

    ; try to enable the a20 line with the bios
    mov ax, 0x2401
    int 0x15
    call a20check

    cli

    ; try to use the keyboard controller to enable the a20 line
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

    ; try the magic port to enable the a20 line
    in al, 0x92
    or al, 2
    out 0x92, al
    call a20check
    
    ; if none of that works then give up
    jmp fail_a20

    ; load the memory map in from the bios
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

    ; put the number of entries at the top of conventional bios memory
    ; all the entries are stored below it in memory
    mov [es:0xFFFF - 2], si

    ; load the gdt for 32 bit stub
    cli
    lgdt [gdt32]

    ; enable protected mode
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
    .size: db 0x10 ; size of the struct, always 0x10
    .zero: db 0 ; zero
    .sectors: dw BOOT_SECTORS ; number of sectors to read
    .addr: dd bootend ; output address
    .start: dq 1 ; sector to start reading at

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

    ; at this point we might actually need a stack
    mov ss, ax
    mov esp, 0x7C00

    ; wipe any existing characters on the vga text buffer
    mov ecx, 1000
.wipe:
    mov dword [0xB8000 + ecx * 4], 0
    dec ecx
    jnz .wipe
    mov dword [0xB8000], 0

    ; check for cpuid using flags
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
    jb fail_cpuid_ext

    ; check for long mode using extended cpuid
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz fail_long

    ; time to build the page tables to get into long mode
    ; pdpt -> pd -> pt -> paddr
    mov ebx, 0x1000
    
    ; ebp contains the pml4
    call bump
    mov ebp, eax

    ; edx contains the pdpt
    call bump
    mov edx, eax

    ; ecx contains the pd
    call bump
    mov ecx, eax

    ; ebx contains the pt
    call bump
    mov ebx, eax

    ; mark pdpt as present and read/write and put it in the pml4
    or edx, 3
    mov dword [ebp], edx
    mov dword [ebp], 0
    and edx, ~3

    ; mark pd as present and read/write and put it into pdpt
    or ecx, 3
    mov dword [edx], ecx
    mov dword [edx + 4], 0
    and ecx, ~1

    ; mark pt as read/write and put it in pd
    or ebx, 3
    mov dword [ecx], ebx
    mov dword [ecx + 4], 0
    and ebx, ~3

    xor eax, eax
    xor ecx, ecx
.loop:
    mov esp, eax
    or esp, 3
    mov dword [ebx + ecx * 4], esp
    mov dword [ebx + ecx * 4 + 4], 0

    add eax, 0x1000
    inc ecx
    cmp ecx, 513
    jl .loop

    ; load the pdpt into cr3 so the cpu can use it
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    mov cr3, ebp

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [high_gdt]
    jmp (gdt64.code - gdt64):start64

%macro error 2
fail_%1:
    mov edi, .msg
    jmp panic32
    .msg: db %2, 0
%endmacro

error cpuid, "cpuid instruction missing"
error cpuid_ext, "extended cpuid missing"
error long, "long mode not supported"

base: dd PT_ADDR

; bump allocate
; ebx = amount to bump by
; eax = allocated memory
bump:
    mov eax, [base]
    add [base], ebx
    ret

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

gdt64:
    dw 0xFFFF
    dw 0
    db 0
    db 0
    db 0
    db 0
.code:
    dw 0
    dw 0
    db 0
    db 10011010b
    db 00100000b
    db 0
.data:
    dw 0
    dw 0
    db 0
    db 10010010b
    db 0
    db 0
.end:

align 16
low_gdt:
    dw gdt64.end - gdt64 - 1
    dd gdt64

align 16
high_gdt:
    dw gdt64.end - gdt64 - 1
    dq gdt64



bits 64
section .boot64
start64:
    jmp $
    ; remap kernel to 64 bit higher half
    call kmain
