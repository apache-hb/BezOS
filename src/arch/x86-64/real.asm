extern start16

global panic

extern KERNEL_SECTORS
extern KERNEL_END
extern LOW_MEMORY

bits 16
section .real
    entry:
        ; we will use the 30KB of memory under the kernel as the stack for now
        mov sp, 0x7C00
        
        mov ax, 0xFFFF
        mov fs, ax
        
        xor ax, ax

        ; zero all the segments
        mov ds, ax
        mov es, ax
        mov ss, ax

    load:
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

        ; target is the sector after this one
        mov bx, 0x7E00 
        
        ; number of sectors to read from
        mov al, KERNEL_SECTORS
        
        ; read from first cylinder
        mov ch, 0

        ; read from first head
        mov dh, 0

        ; read starting at the second sector because the first one is already loaded
        mov cl, 2

        ; read operation
        mov ah, 2

        ; execute disk operation
        int 0x13

        add dword [LOW_MEMORY], KERNEL_END

        ; technically we should check if the disk operation worked
        ; but for some reason the carry flag is set no matter what

        ; jump to bootloader
        jmp start16

    times 510 - ($-$$) db 0
    dw 0xAA55