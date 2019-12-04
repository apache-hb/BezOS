OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start32)

KERNEL_LMA = 0x100000;
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

SECTIONS {
    . = KERNEL_VMA;

    _start32 = .;

    .text : AT(ADDR(.text) - KERNEL_VMA) {
        *(.text*)
        . = ALIGN(4K);
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata*)
        . = ALIGN(4K);
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss*)
        *(COMMON)
        . = ALIGN(4K);
    }

    /DISCARD/ : {
        *(.comment)
    }
}