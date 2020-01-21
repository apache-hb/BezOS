global start16

extern LOW_MEMORY
extern start32

bits 16
section .boot
    header:
        jmp start16
        times 3-($-$$) db 0x90

        .oem_name:          db "jeff.fat"
        .bytes_per_sect:    dw 512
        .sect_per_cluster:  db 1
        .reserved_sects:    dw 1
        .nut_fat:           db 2
        .num_root_dirs:     dw 224
        .num_sects:         dw 2880
        .media_type:        db 0xF0
        .num_fat_sects:     dw 9
        .sects_per_track:   dw 18
        .num_heads:         dw 2
        .num_hidden_sects:  dd 0
        .num_sects_huge:    dd 0
        .drive_num:         db 0
        .reserved:          db 0
        .signature:         db 0x29
        .volume_id:         dd 0x2D7E5A1A
        .volume_label:      db "NO NAME    "
        .file_type:         db "FAT12   "

    start16:
<<<<<<< HEAD
=======
        ; when we get here only the register dl has a known value
        ; the value of the boot drive
        ; so we need to track what are in the registers to make sure we dont accidentally read bad data

        ; lets give outselves a small stack below the kernel
        mov sp, 0x7C00

        ; set fs to 0xFFFF
        mov bx, 0xFFFF
        mov fs, bx

        ; zero bx 
        xor bx, bx

        ; zero the stack segment register so we have an absolute offset
        mov ss, bx

        ; zero the data and the extra segment
        mov ds, bx
        mov es, bx

        ; load the rest of the kernel in from the disk
    load_kernel:

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

        ; we need to read into memory right after this section
        mov bx, 0x7E00

        ; we need all the kernel sectors
        ; KERNEL_SECTORS is calculated by the linker script to tell us
        ; the size of the kernel in 512 byte sectors
        mov al, KERNEL_SECTORS

        ; everything is on the first cylinder for now
        ; TODO: what happens when we use more than 1440 sectors?
        mov ch, 0

        ; everything is read from the first head of the disk drive
        mov dh, 0

        ; we start from the second sector because the bios loads the first for us
        mov cl, 2

        ; ah = 2 means we want to read from disk so we dont overwrite it
        mov ah, 2

        ; then we use the disk interrupt to perform disk io
        int 0x13

        ; then we set the used memory to the end of the kernel
        add dword [LOW_MEMORY], KERNEL_END

        ; enable the a20 memory line to disable memory wrapping
        ; this is needed to get into protected mode
    enable_a20:
>>>>>>> parent of e4cb183... add some more stuff
        ; check if the a20 line is already enabled
        call a20check

        ; try to enable through the bios
        .bios:

            ; try and enable the a20 line through the bios call
            mov ax, 0x2401
            int 0x15

            ; check again for the a20 line
            call a20check

        ; if the bios doesnt work try the keyboard controller
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
        ; if the keyboard doesnt work then try port io
        .port:
            in al, 0xEE
            call a20check

        ; if the port io doesnt work then give up
        .fail:
            ; just panic with a message
            mov si, a20_msg
            call panic

        ; collect e820 bios memory map
        ; this can only be done in real mode so do it now
        ; the less mode switching later on the better

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

    collect_e820:
            mov ebx, 0
            mov edi, [LOW_MEMORY]

            mov dword [edi], 0
            add edi, 4

            jmp .begin
        .next:
            add edi, ecx
            mov dword [edi], ecx
            add edi, 4
        .begin:
            mov eax, 0xE820
            mov edx, 0x534D4150
            mov ecx, 24

            int 0x15

            cmp ecx, 20
            je .fix_size

            cmp ecx, 24
            jne .fail
        .fix_size:
            cmp eax, 0x534D4150
            jne .fail

            cmp ebx, 0
            jne .next
        .end:
            mov [LOW_MEMORY], edi
            jmp enter_prot
        .fail:
            ; if the e820 mapping fails then panic
            mov si, e820_msg
            call panic

    enter_prot:
        cli

        ; load the global descriptor table
        lgdt [descriptor]

        ; enable the protected mode bit
        mov eax, cr0
        or eax, 1
        mov cr0, eax

        ; set ax to the data descriptor
        mov ax, (descriptor.data - descriptor)

        ; jump into protected mode code
        jmp (descriptor.code - descriptor):start32

    ; print a message to the console
    ; si = pointer to message
    print:
        cld ; clear the direction
        mov ah, 0x0E ; print magic
    .put:
        lodsb ; load the next byte
        or al, al ; if the byte is zero
        jz .end ; if it is zero then return
        int 0x10 ; then print the charater
        jmp .put ; then loop
    .end:
        ret ; return from function

    ; print a message then loop forever
    ; si = pointer to message
    panic:
        call print
    .end:
        cli
        hlt
        jmp .end

    a20wait:
        in al, 0x64
        test al, 2
        jnz a20wait
        ret

    a20wait2:
        in al, 0x64
        test al, 1
        jz a20wait2
        ret

    ; check if the a20 line is enabled by using the memory wrap around
    a20check:
        mov ax, word [es:0x7DFE] ; 0x0000:0x7DFE is the same address as 0xFFFF:0x7E0E due to wrap around
        cmp word [fs:0x7E0E], ax ; this is the boot signature, we compare these to see if the wraparound is disabled
        je .change

    .change:
        mov word [es:0x7DFE], 0x6969 ; change the values to make sure the memory wraps around
        cmp word [fs:0x7E0E], 0x6969
        jne collect_e820
    .end:
        mov word [es:0x7DFE], ax ; write the boot signature back for next time
        ret

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

    a20_msg: db "failed to activate a20 line", 0
    e820_msg: db "failed to collect e820 memory map", 0
<<<<<<< HEAD
=======
    here_msg: db "got here", 0

    global LOW_MEMORY
    LOW_MEMORY: dd 0x7E00

    times 510 - ($-$$) db 0
    dw 0xAA55
>>>>>>> parent of e4cb183... add some more stuff
