extern kmain

extern SECTORS
extern END

extern KERN_ADDR
extern KERN_SIZE

extern BSS_ADDR
extern BSS_DWORDS

struc partition
    .attrib: resb 1
    .start: resb 3
    .type: resb 1
    .end: resb 3
    .lba: resd 1
    .sectors: resd 1
endstruc

bits 16
section .real
    init:
        cli
        jmp 0:start16
    start16:
        xor ax, ax
        mov ds, ax
        mov ss, ax
        mov sp, 0x7C00

        ; setup these segments so we can check if the a20 line is enabled later on
        mov es, ax
        not ax
        mov fs, ax
        sti

        clc

        mov ah, 0x41
        mov bx, 0x55AA
        int 0x13

        jc fail_ext
        cmp bx, 0xAA55
        jc fail_ext


        ; TODO: we should only load in the bootloader here
        mov word [dap.sectors], SECTORS
        mov ah, 0x42
        mov si, dap
        int 0x13

        jc fail_disk

    a20enable:
        call a20check

    .bios:
        mov ax, 0x2401
        int 0x15

        call a20check

    .kbd:
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
    .fail:
        jmp fail_a20


    a20check:
        mov ax, word [es:0x7DFE]
        cmp word [fs:0x7E0E], ax
        je .change
    .change:
        mov word [es:0x7DFE], 0x6969
        cmp word [fs:0x7E0E], 0x6969
        jne e820map
    .end:
        mov word [es:0x7DFE], ax
        ret
        
    a20wait:
        in al, 0x64
        test al, 2
        jnz a20wait
        ret

    a20wait2:
        in al, 0x65
        test al, 1
        jz a20wait2
        ret



    fail_ext:
        mov si, .msg
        jmp panic
        .msg: db "disk extensions not support", 0

    fail_disk:
        mov si, .msg
        jmp panic
        .msg: db "failed to read from disk", 0

    fail_a20:
        mov si, .msg
        jmp panic
        .msg: db "failed to enable a20 line", 0

    fail_e820:
        mov si, .msg
        jmp panic
        .msg: db "failed to read e820 map", 0



    panic:
        cld
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

    align 4
    dap:
        .len: db 0x10
        .zero: db 0
        .sectors: dw 0
        .offset: dw signature + 2
        .segment: dw 0
        .addr: dq 1

    align 4
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

    times 440 - ($-$$) db 0
    mbr:
        .udid: dd 0
        .zero: dw 0
        .pte0: 
            istruc partition 
            iend
        .pte1:
            istruc partition 
            iend
        .pte2:
            istruc partition 
            iend
        .pte3:
            istruc partition 
            iend
    signature: dw 0xAA55

    e820_ptr: dd 0
    e820_num: dd 0

    e820map:
        mov edi, END
        xor ebx, ebx

        jmp .begin

    .next:
        add edi, 24
        inc dword [e820_num]
    .begin:
        mov eax, 0xE820
        mov edx, 0x534D4150
        mov ecx, 24

        int 0x15

        ; make sure the e820 call was valid
        cmp eax, 0x534D4150
        jne fail_e820

        cmp ecx, 20
        je .fix_size

        jmp .next

    .fix_size:
        mov dword [edi+20], 0
        cmp ebx, 0
        jne .next
    .end:
        mov dword [e820_ptr], edi
        jmp enter_prot

    enter_prot:
        cli

        lgdt [descriptor]

        mov eax, cr0
        or eax, 1
        mov cr0, eax

        mov ax, (descriptor.data - descriptor)
        jmp (descriptor.code - descriptor):start32

bits 32
section .prot
    start32:
        mov ds, ax
        mov ss, ax

        xor ax, ax

        mov gs, ax
        mov fs, ax
        mov es, ax

        mov esp, 0x7C00

        call vga_init
        
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

        ; make sure we have extended cpuid
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000001
        jb fail_long

        ; make sure we have long mode
        mov eax, 0x80000001
        cpuid
        test edx, 1 << 29
        jz fail_long

        ; next we need to setup paging for the rest of the kernel
        ; eventually we need to load the kernel in above the 1mb mark
        ; this will be possible at the point we get usb drivers
        ; TODO: the plan
        ;   - minimal bootloader thing
        ;       - bump allocator
        ;       - drivers to read from storage
        ;           - usb
        ;           - disk
        ;           - sata
        ;           - nvme
        ;       - rest of kernel has to be stored somewhere
        ;           - probably right after the bootloader in memory
        ;           - either store it as a flat binary or in some custom format
        ; for now lets just slap shit together and hope it works

        ; clear the bss
        call bss_init

        call paging_init

        mov ecx, 0xc0000080
        rdmsr
        or eax, (1 << 8) | (1 << 0)
        wrmsr

        mov eax, cr0
        or eax, (1 << 31) | (1 << 0)
        mov cr0, eax

        mov eax, cr4
        or eax, (1 << 7)
        mov cr4, eax

        jmp enter_long
    enter_long:
        jmp $


    paging_init:
        mov     eax, 3
        mov     ecx, pml1
    .next:
        mov     dword [ecx], eax
        add     eax, 4096
        mov     dword [ecx + 4], 0
        add     ecx, 8
        cmp     eax, 2097155
        jne     .next
        mov     eax, pml1
        mov     dword [pml2+4], 0
        mov     dword [pml3+4], 0
        or      eax, 3
        mov     dword [pml2], eax
        mov     eax, pml2
        or      eax, 1
        mov     dword [pml3], eax
        mov     eax, pml3
        mov     eax, cr4
        bts     eax, 5
        mov     cr4, eax
        or      eax, 1
        mov     cr3, eax
        mov     eax, cr0
        or      eax, 2147483648
        mov     cr0, eax

        ret

    bss_init:
        mov     eax, BSS_DWORDS
        test    eax, eax
        jle     .end
        xor     eax, eax
    .next:
        mov     ecx, dword [BSS_ADDR]
        mov     dword [ecx + 4*eax], 0
        inc     eax
        cmp     eax, BSS_DWORDS
        jl      .next
    .end:
        ret

    ; clear the vga text buffer so we cant print messages to it
    vga_init:
        mov eax, -4000
    .next:
        mov word [eax + 757664], 0
        add eax, 2
        jne .next
        ret

    vga_panic:
        cld
        mov ebx, 0xB8000
    .put:
        lodsb
        or al, al
        jz .end
        mov ah, 7
        mov word [ebx], ax
        add ebx, 2
        jmp .put
    .end:
        cli
        hlt
        jmp .end

    fail_long:
        mov si, .msg
        jmp vga_panic
        .msg: db "cpu does not not support long mode", 0

    fail_cpuid:
        mov si, .msg
        jmp vga_panic
        .msg: db "cpu does not support cpuid", 0

    descriptor64:
        .null:
        .kdata:
        .kcode:
        .udata:
        .ucode:
        .tss:
    .end:

bits 64
section .long
    start64:
        mov rax, kmain
        call rax

section .bss
    pml1: resb 4096
    pml2: resb 4096
    pml3: resb 4096
