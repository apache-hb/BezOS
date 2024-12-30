.section .smp.info

KmSmpInfoStart:

.align 16
KmSmpStartAddress:
    .quad 0 // Address of the smp startup C++ code

    // PAT MSR value from the BSP we need to copy before enabling paging
KmSmpPageAttributeTable0:
    .long 0
KmSmpPageAttributeTable1:
    .long 0
KmSmpPml4:
    .long 0 // Address of the PML4 table
KmSmpHhdmOffset:
    .quad 0 // Offset of the HHDM from the start of the kernel

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
.equ KmPat0Address, 0x7000 + (KmSmpPageAttributeTable0 - KmSmpInfoStart)
.equ KmPat1Address, 0x7000 + (KmSmpPageAttributeTable1 - KmSmpInfoStart)
.equ KmPml4Address, 0x7000 + (KmSmpPml4 - KmSmpInfoStart)
.equ KmHhdmOffsetAddress, 0x7000 + (KmSmpHhdmOffset - KmSmpInfoStart)

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
    // Setup protected mode data segment
    mov $KmProtectedModeData, %ax

    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    // Clear the cr4 register, we don't want anything set yet
    xor %eax, %eax
    mov %eax, %cr4

    // Copy the PAT MSR value from the BSP
    mov KmPat0Address, %eax
    mov KmPat1Address, %edx
    mov $0x277, %ecx
    wrmsr

    // Enable PAE - CR4.PAE (bit 5)
    // Enable WP - CR4.WP (bit 16)
    mov %cr4, %eax
    or $(1 << 5) | (1 << 16), %eax
    mov %eax, %cr4

    // Enable IA32_EFER.LME (bit 8) to start entering long mode
    mov $0xc0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    xor %edx, %edx
    wrmsr

    // Setup cr3 to point to the PML4 table
    mov KmPml4Address, %eax
    mov %eax, %cr3

    // Enable paging - CR0.PG (bit 31)
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    jmp $KmLongModeCode, $KmSmpTrampoline64

.code64
.align 16
KmSmpTrampoline64:
    jmp KmSmpTrampoline64
