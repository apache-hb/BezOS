.section .smp.info

KmSmpInfoStart:

.align 16
KmSmpStartAddress:
    .quad 0 // Address of the smp startup C++ code

.align 16
KmGdtBegin:
    .quad 0 // Null
    .quad 0 // Real mode code
    .quad 0 // Real mode data
    .quad 0 // 32-bit code
    .quad 0 // 32-bit data
    .quad 0 // 64-bit code
    .quad 0 // 64-bit data
KmGdtEnd:

.align 16
KmGdtValue:
    .word KmGdtEnd - KmGdtBegin - 1
    .quad KmGdtBegin

.equ KmGdtAddress, 0x7000 + (KmGdtValue - KmSmpInfoStart)

.equ KmRealModeCode, 8
.equ KmRealModeData, 16
.equ KmProtectedModeCode, 24
.equ KmProtectedModeData, 32
.equ KmLongModeCode, 40
.equ KmLongModeData, 48

.section .smp.startup,"a"

.align 16
.code16
KmSmpTrampoline:
    cli
    cld
    xor %ax, %ax
    mov %ax, %ds
    lgdt KmGdtAddress

    // Enable protected mode - CR0.PE (bit 0)
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    jmp $KmProtectedModeCode, $KmSmpTrampoline32

.code32
.align 32
KmSmpTrampoline32:
    1: jmp 1b

    // Setup protected mode data segment
    mov $KmProtectedModeData, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    xor %eax, %eax
    lldt %ax

    jmp KmSmpTrampoline32

.code64
.align 16
KmSmpTrampoline64:
