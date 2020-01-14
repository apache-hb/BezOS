; number of 512 byte sectors the kernel takes up
extern KERNEL_SECTORS

; size of the kernel aligned to page
extern KERNEL_END

; 32 bit protected mode code
extern start32

section .boot
bits 16
    boot:
        jmp start
        times 3-($-$$) db 0x90

        .oem_name: db "mkfs.fat"
        .bytes_per_sector: dw 512
        .sect_per_cluster: db 1
        .reserved_sects: dw 1
        .num_fat: db 2
        .num_root_dirs: dw 224
        .num_sects: dw 2880
        .media_type: db 0xF0
        .num_fat_sects: dw 9
        .sects_per_track: dw 18
        .num_heads: dw 2
        .num_hidden_sects: dd 0
        .num_sects_huge: dd 0
        .drive_num: db 0
        .reserved: db 0
        .signature: db 0x29
        .volume_id: dd 0x2D7E5A1A
        .volume_label: db "NO NAME    "
        .file_type: db "FAT12   "

    start:
        ; wipe all the gprs
        xor ebx, ebx
        xor ecx, ecx
        xor edx, edx
        
        ; set stack pointer
        mov sp, 0x7C00

        ; zero the rest of the registers
        mov ss, bx
        mov ds, bx
        mov es, bx

        ; set fs to 0xFFFF
        mov bx, 0xFFFF
        mov fs, bx

        ; we need to load in the next sectors after the kernel
        ; we do this by passing alot of data into registers
        ; we set
        ; bx = memory to start loading disk data into
        ; al = number of 512 byte sectors to load
        ; ch = the cylinder to read from
        ; dh = the head to read from
        ; cl = the first sector we want to read from
        ; ah = 2 which is the number chosen to read from disk
        ;
        ; then once we setup all the registers we 
        ; call the disk interrupt to execute our disk query

        mov bx, [LOW_MEMORY] ; we need to load them into the sectors after the boot sector
        mov al, KERNEL_SECTORS ; we need all the sectors
        ; TODO: account for KERNEL_SECTORS > 1440
        mov ch, 0 ; take them from the first cylinder
        mov dh, 0 ; and the first head
        mov cl, 2 ; we start reading from the second sector because the first was already loaded
        mov ah, 2 ; and ah = 2 means we want to read in so we dont overwrite the disk 
        int 0x13 ; then we do the disk interrupt to perform the action

        ; we have used this so mark it as used
        add dword [LOW_MEMORY], KERNEL_END

        ; clear the direction
        cld

        ; try and enable the a20 line
        call .a20_enable

        ; if we cant then fail out
        ; mov si, here
        ; jmp .a20_fail

        ; if we get here that means that the a20 gate was successfully enabled

        ; next we have to do the whole memory mapping thing so that later code
        ; knows where we can and cant put stuff
        ; overwriting the bios isnt something we want to do really

        ; so lets begin memory mapping
        ; the bios has an interrupt for this and a function code (0xE820)
        ; this returns a small struct with all the bits we need for our first memory map
        ; we can always read more complicated data in later but for now we dont need it
        ; and if its good enough for linux its probably good enough for us

        ; we want to layout the tables like so
        ;
        ; [entry1(20/24) | u32 size]
        ; [entry2(20/24) | u32 size]
        ; [entryN(20/24) | u32 size]
        ;
        ; this means we can read in 4 bytes from the top of our "stack" of tables
        ; then using those 4 bytes we know how much to read afterwards
        ; we store the size for each entry because im not sure if
        ; the standards define whether sizes can be mixed or not
        ; so better safe than sorry

        ; continuation byte
        mov ebx, 0

        ; output location
        mov edi, [LOW_MEMORY]

        ; add a 0 to signify that this is the end of the stack of memory maps
        mov dword [edi], 0
        add edi, 4

        jmp .begin_map

    .next_map:
        add edi, ecx
        mov dword [edi], ecx
        add edi, 4
    .begin_map:
        
        mov eax, 0xE820 ; E820 function code
        mov edx, 0x534D4150 ; signature (SMAP)
        mov ecx, 24 ; desired output size (it may return 20 bytes if it doesnt support acpi3 attributes)

        int 0x15 ; memory map interrupt

        ; make sure ecx is either 20 or 24 (these are the only valid sizes)
        cmp ecx, 20
        je .correct_size

        cmp ecx, 24
        jne .e820_size

    .correct_size:

        cmp eax, 0x534D4150
        jne .e820_fail

        ; continuation is put into ebx, if its not 0 there are more sections to read
        cmp ebx, 0
        jne .next_map

    .end_map:

        mov [LOW_MEMORY], edi

        cli ; clear interrupts

        ; enable protected mode bit
        mov eax, cr0 
        or eax, 1
        mov cr0, eax

        ; load the global descriptor table
        lgdt [descriptor]

        ; push the data segment offset to the stack so we 
        ; can use it in protected mode for segmentation
        push (descriptor.data - descriptor)
        jmp (descriptor.code - descriptor):start32

        
        ; a20 stuff

        ; check if the a20 line is enabled
        ; if it is enabled the function will jump back to boot
    .a20_check:
        mov ax, word [es:0x7DFE] ; test if memory wraps around
        cmp word [fs:0x7E0E], ax

        ; if it does wrap then the a20 line isnt enabled 
        je .done

        ; if it doesnt then change the values and just make sure
    .change:
        mov word [es:0x7DFE], 0x6969
        cmp word [fs:0x7E0E], 0x6969
        ; if we're really sure then go back to main
        jne .a20_done

        ; otherwise just return
    .done:
        ; restore boot signature
        mov word [es:0x7DFE], ax
        ret

        ; try and enable the bios
    .a20_enable:


    .a20_done:
        ret
        
        ; enable the a20 line through the bios
    .a20_bios:

        mov ax, 0x2403
        int 0x15
        jb .a20_kbd

        cmp ah, 0
        jb .a20_kbd

        dec ax ; 0x2402
        int 0x15
        jb .a20_kbd

        cmp ah, 0
        jnz .a20_kbd

        cmp al, 1
        jz .a20_kbd

        dec ax ; 0x2401
        int 0x15
        jb .a20_kbd

        cmp ah, 0
        jz .a20_kbd

        call .a20_check
        jmp .a20_kbd
        
        ; enable the a20 line through the keyboard
    .a20_kbd:
        mov si, here
        call real_print
        cli

        sti
        call .a20_fast

        ; enable the a20 line through the fast port
    .a20_fast:

        call .a20_fail
        ret

    .a20_fail:
        mov si, fail_a20
        jmp real_panic

    .e820_size:
        mov si, size_e820
        jmp real_panic

    .e820_fail:
        mov si, fail_e820
        jmp real_panic

    ; print a message
    ; set si = message
    real_print:
        cld
        mov ah, 0x0E
    .put:
        lodsb ; load next character into al
        or al, al ; if the charater is null
        jz .end ; then hang at end
        int 0x10 ; otherwise print character
        jmp .put ; then loop again
    .end:
        ret

    ; real mode kernel panic, prints message then hangs
    ; set si = message
    real_panic:
        call real_print
    .end:
        cli
        hlt
        jmp .end

    fail_a20: db "failed to enable a20 line", 0
    fail_e820: db "e820 memory mapping failed", 0
    size_e820: db "e820 structure was the wrong size", 0

    here: db "here", 0

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

    
    ; pointer to the 480KB above the kernel
    global LOW_MEMORY
    LOW_MEMORY: dd 0x7E00

    ; byte 511, 512 of the first sector need to have this magic
    ; to mark this as a bootable sector
    times 510 - ($-$$) db 0
    dw 0xAA55


    ; check if the a20 line is enabled
    ; .a20_check:
    ;     mov ax, word [es:0x7DFE] ; if the a20 line is disabled these will wrap around and read the same address
    ;     cmp word [fs:0x7E0E], ax ; we read the boot signature because we're already loaded and nuking it isnt a problem

    ;     je .change

    ; .disabled:
    ;     ; restore the boot signautre
    ;     mov word [es:0x7DFE], ax
        
    ;     ; return
    ;     ret

    ; .change:
    ;     mov word [es:0x7DFE], 0x6969
    ;     cmp word [fs:0x7E0E], 0x6969
    ;     jne .a20_enabled
    ;     jmp .disabled

    ; ; try and enable the a20 line using the bios
    ; .a20_bios:
    ;     ; check if its enabled
    ;     call .a20_check
    ;     ; try and enable the a20
    ;     mov ax, 0x2401
    ;     int 0x15

    ;     ; try again
    ;     call .a20_check
    ;     ret
    
    ; ; try and enable the a20 line using the keyboard controller
    ; .a20_kbd:
    ;     cli

    ;     call .a20wait
    ;     mov al, 0xAD
    ;     out 0x64, al

    ;     call .a20wait
    ;     mov al, 0xD0
    ;     out 0x64, al

    ;     call .a20wait2
    ;     mov al, 0x60
    ;     push eax

    ;     call .a20wait
    ;     mov al, 0xD1
    ;     out 0x64, al

    ;     call .a20wait
    ;     pop eax
    ;     or al, 2
    ;     out 0x60, al

    ;     call .a20wait
    ;     mov al, 0xAE
    ;     out 0x64, al

    ;     call .a20wait
    ;     sti

    ;     call .a20_check

    ;     ret

    ; .a20wait:
    ;     in al, 0x64
    ;     test al, 2
    ;     jnz .a20wait
    ;     ret

    ; .a20wait2:
    ;     in al, 0x64
    ;     test al, 1
    ;     jz .a20wait2
    ;     ret

    ; ; try and enable the a20 line using the fast 0xEE port
    ; .a20_fast:
    ;     in al, 0xEE
    ;     call .a20_check
    ;     ret