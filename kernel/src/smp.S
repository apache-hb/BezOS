.section .smp.info

.align 16
KmRealModeGdtBegin:
    .quad 0 // null
    .quad 0x00cf9a000000ffff // code
    .quad 0x00cf92000000ffff // data
    .quad 0x00cf9a000000ffff // tss
KmRealModeGdtEnd:
KmRealModeGdtValue:
    .word KmRealModeGdtEnd - KmRealModeGdtBegin - 1
    .quad KmRealModeGdtBegin

.equ KmRealModeGdtAddress, 0x7000 + (KmRealModeGdtValue - KmRealModeGdtBegin)

.align 16
KmSmpInfoBegin:
KmSmpInfo_Pml4:
    .long 0
KmSmpInfo_Stack:
    .long 0
KmSmpInfo_StackSize:
    .long 0
KmSmpInfo_EntryPoint:
    .long 0
KmSmpInfoEnd:

.equ KmPml4Address, 0x7000 + (KmSmpInfo_Pml4 - KmSmpInfoBegin)
.equ KmStackAddress, 0x7000 + (KmSmpInfo_Stack - KmSmpInfoBegin)
.equ KmStackSizeAddress, 0x7000 + (KmSmpInfo_StackSize - KmSmpInfoBegin)
.equ KmEntryPointAddress, 0x7000 + (KmSmpInfo_EntryPoint - KmSmpInfoBegin)

.section .smp.startup,"awx",@progbits

.align 16
.code16
KmSmpTrampoline:
    cli
    cld
    xor %ax, %ax
    mov %ax, %ds
    lgdt KmRealModeGdtAddress
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0
    jmp $0x08, $KmSmpTrampoline32

.code32
.align 16
KmSmpTrampoline32:
    jmp KmSmpTrampoline32

.code64
.align 16
KmSmpTrampoline64:
