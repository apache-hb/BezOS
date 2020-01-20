; number of 512 byte sectors the kernel takes up
extern KERNEL_SECTORS

; size of the kernel aligned to page
extern KERNEL_END

; 32 bit protected mode code
extern start16

global LOW_MEMORY

bits 16
section .real
    boot:
        ; when we get here only the register dl has a known value
        ; the value of the boot drive
        ; so we need to track what are in the registers to make sure we dont accidentally read bad data

        ; lets give outselves a small stack below the kernel
        mov sp, 0x7C00

        ; set fs to 0xFFFF
        mov ax, 0xFFFF
        mov fs, ax

        ; zero bx 
        xor ax, ax

        ; zero the stack segment register so we have an absolute offset
        mov ss, ax

        ; zero the data and the extra segment
        mov ds, ax
        mov es, ax

        ; clear carry bit and direction bit
        clc
        cld
        sti

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

        jmp start16

    LOW_MEMORY: dd 0x7C00

    times 510 - ($-$$) db 0
    dw 0xAA55