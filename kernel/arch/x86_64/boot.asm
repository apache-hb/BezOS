
; real mode
; we need to get out of real mode and quickly
; to do this we 
; 1. enable a20 line
; 2. disable interrupts and non maskable ones (we reneable those later)
; 3. load global descriptor table
bits 16
boot16:
    ; check if the a20 gate is supported
    mov ax, 0x2403
    int 0x15

    ; if the a20 gate isnt supported then we wont boot
    jb .a20_unsupported
    cmp ah, 0
    jnz .a20_unsupported

    ; get the current a20 status
    mov ax, 0x2402
    int 0x15

    ; if we couldnt get the a20 status then dont boot
    jb .a20_failed
    cmp ah, 0
    jnz .a20_failed

    ; if the a20 is already activated then skip ahead
    cmp al, 1
    jz .a20_good


    ; activate the a20
    mov ax, 0x2041
    int 0x15

    ; we couldnt activate the a20 so we dont boot
    jb .a20_failed
    cmp ah, 0
    jnz .a20_failed

.a20_unsupported:
    ; dumb hack to show the letter u if a20 is unsupported
    mov al, 'u'
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07

    int 0x10
    jmp $

.a20_failed:
    ; show f if a20 failed
    mov al, 'f'
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07

    int 0x10
    jmp $

.a20_good:
    ; weve disabled the a20 line
    ; time to disable interrupts

    ; disable normal interrupts
    cli

    ; disable non maskable ones
    in al, 0x70
    and al, 0x80
    out 0x70, al

    ; load global descriptor table
    ; TODO

    ; go to protected mode
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; far jump to protected mode code
    jmp 0x08:boot32

; boot sector magic
times 510-($-$$) db 0
db 0x55
db 0xAA

; protected mode
bits 32
boot32:

; long mode
bits 64
boot64: