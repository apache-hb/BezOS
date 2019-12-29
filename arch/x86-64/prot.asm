; protected mode assembly

global start32:function
extern kmain
extern kpanic
extern KERNEL_END

section .protected
; we're in protected mode now, so we can use 32 bit instructions
bits 32
    ; clear a block of memory
    ; ecx = size
    ; edx = address
    ; eax = 0
    clear:
        xor eax, eax
    .next:
        mov [edx+ecx*4], eax
        dec ecx
        jnz .next
        ret

    ; 32 bit start
    start32:
        mov bx, 0x10
        mov ds, bx
        mov es, bx

        mov al, 0xFE
        mov cr0, eax

        pop es
        pop ds
        sti
        ; check for long mode so we can get into it
        ; use cpuid to check for long mode

        ; check if we have cpuid

        ; copy flags to eax
        pushfd
        pop eax

        ; move flags to ecx for later
        mov ecx, eax

        ; flip the id bit
        xor eax, 1 << 21

        ; set flags to eax
        push eax
        popfd

        ; set eax to flags. the bit will be flipped if cpuid is supported
        pushfd
        pop eax

        ; restore flags to ecx
        push ecx
        popfd

        ; if eax and ecx arent equal then we dont have cpuid
        xor eax, ecx
        jz .cpuid_no

        ; check if we support extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb .long_no

        ; if we support extended cpuid we can check if we support long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz .long_no

        ; todo: paging
        ; we need somewhere to put the page tables
        ; lets find where we can put it after the stack
        ; get the end of the kernels memory location and round up to the next 4096 alignment
        mov eax, KERNEL_END
        add eax, 0x1000 - 1
        and eax, ~0x1000

        ; add eax, 0x1000 * 3

        ; kernel
        ; ----
        ; pages
        ;   p4
        ;   p3
        ;   p2

        ; TODO: all this, dear god indirection is painful
        ; mov [tables.p2], eax
        
        ; add eax, 0x1000
        ; mov [tables.p3], eax

        ; add eax, 0x1000
        ; mov [tables.p4], eax

        ; mov eax, [tables.p3]
        ; mov eax, [eax]
        ; or eax, 11b
        ; mov [tables.p4], eax

        ; mov eax, [tables.p2]
        ; mov eax, [eax]
        ; or eax, 11b
        ; mov [tables.p3], eax

        ; enable paging
        ; put p4 into cr3 for the cpu to use
        mov eax, [tables.p4]
        mov cr3, eax

        ; enable pysical address extension
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax        

        ; set long mode bit
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        ; enable paging
        mov eax, cr0
        or eax, 1 << 8
        mov cr0, eax

        ; go to long mode


        ; go to real 64 bit code
        lgdt [descriptor64.ptr]
        call descriptor64.code:kmain

    ; todo: panic
    .cpuid_no:
        mov dword [0xb8000], 0x2f4b2f4f
    .long_no:
        mov dword [0xb8000], 0x2f4b2f4f
    panic:

descriptor64:
    .null:
        dw 0xFFFF
        dw 0
        db 0
        db 0
        db 1
        db 0
    .code:
        dw 0
        dw 0 
        db 0
        db 10011010b
        db 10101111b
        db 0
    .data:
        dw 0
        dw 0
        db 0
        db 10010010b
        db 0
        db 0
    .end:
    .ptr:
        dw descriptor64 - descriptor64.end - 1
        dq descriptor64

tables:
    .p4: dd 0
    .p3: dd 0
    .p2: dd 0

