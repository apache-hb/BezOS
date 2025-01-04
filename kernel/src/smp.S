#include "gdt.h"

// This section will not be part of the final binary.
// It is here to calculate the offsets of each field in the smp info structure.
// The smp startup routines in smp.cpp will create and fill out data
// and place it at 0x7000.

.section .smp.info

.align 16

KmSmpInfoStart:

// Address of the smp startup C++ code
KmSmpStartAddress:
    .quad 0

// PAT MSR value from the BSP we need to copy before enabling paging.
.align 8
KmSmpPageAttributeTable0:
    .long 0
KmSmpPageAttributeTable1:
    .long 0

// Physical address of the top level page table.
.align 4
KmSmpPml4:
    .long 0

// Base address of the stack for this AP.
.align 8
KmSmpStack:
    .quad 0

.align 16
KmGdtBegin:
    .space (GDT_COUNT * 0x8), 0x0
KmGdtEnd:

.align 16
KmGdtValue:
    .word KmGdtEnd - KmGdtBegin - 1
    .quad KmGdtBegin

.equ KmInfoAddress, 0x7000
.equ KmStartAddress, 0x7000 + (KmSmpStartAddress - KmSmpInfoStart)
.equ KmGdtAddress, 0x7000 + (KmGdtValue - KmSmpInfoStart)
.equ KmPat0Address, 0x7000 + (KmSmpPageAttributeTable0 - KmSmpInfoStart)
.equ KmPat1Address, 0x7000 + (KmSmpPageAttributeTable1 - KmSmpInfoStart)
.equ KmPml4Address, 0x7000 + (KmSmpPml4 - KmSmpInfoStart)
.equ KmSmpStackAddress, 0x7000 + (KmSmpStack - KmSmpInfoStart)

.equ KmRealModeCode, (GDT_16BIT_CODE * 0x8)
.equ KmRealModeData, (GDT_16BIT_DATA * 0x8)
.equ KmProtectedModeCode, (GDT_32BIT_CODE * 0x8)
.equ KmProtectedModeData, (GDT_32BIT_DATA * 0x8)
.equ KmLongModeCode, (GDT_64BIT_CODE * 0x8)
.equ KmLongModeData, (GDT_64BIT_DATA * 0x8)

.section .smp.startup,"a"

.align 16
.code16
KmSmpTrampoline:
    cli
    cld
    xor %ax, %ax
    mov %ax, %ds
    lgdt KmGdtAddress

    // Enable Protected Mode - CR0.PE (bit 0)
    // Enable Numeric Error - CR0.NE (bit 5)
    mov %cr0, %eax
    or $(1 << 0) | (1 << 5), %eax
    mov %eax, %cr0

    // Long jump to protected mode code
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
    mov %ax, %ss

    // Copy the PAT MSR value from the BSP
    // x86 requires that all cores have the same PAT
    mov KmPat0Address, %eax
    mov KmPat1Address, %edx
    mov $0x277, %ecx
    wrmsr

    // Enable PAE - CR4.PAE (bit 5)
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    // Enable IA32_EFER.LME (bit 8) to start entering long mode
    // Enable IA32_EFER.NXE (bit 11) to enable execute disable pages
    mov $0xc0000080, %ecx
    rdmsr
    or $(1 << 8) | (1 << 11), %eax
    xor %edx, %edx
    wrmsr

    // Enable WP - CR0.WP (bit 16)
    mov %cr0, %eax
    or $(1 << 16), %eax
    mov %eax, %cr0

    // Setup cr3 to point to the PML4 table
    mov KmPml4Address, %eax
    mov %eax, %cr3

    // Enable paging - CR0.PG (bit 31)
    // Disable CR0.CD, and CR0.NW (bits 30 and 29)
    mov %cr0, %eax
    or $(1 << 31), %eax
    and $~((1 << 30) | (1 << 29)), %eax
    mov %eax, %cr0

    jmp $KmLongModeCode, $KmSmpTrampoline64

.code64
.align 16
KmSmpTrampoline64:
    // Load the stack we were given
    mov KmSmpStackAddress, %rsp

    // Long jump to reload CS
    pushq $KmLongModeCode
    lea KmReloadCs(%rip), %rax
    push %rax
    lretq
KmReloadCs:

    // Clear out segments we don't need
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    // Load the data segment into ss
    mov $KmLongModeData, %ax
    mov %ax, %ss

    // Call back into C++ code

    mov $KmInfoAddress, %rdi
    mov KmStartAddress, %rax
    call *%rax
