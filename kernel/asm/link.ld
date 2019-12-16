OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_boot)

KERNEL_LMA = 0x0000000000100000;
KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = KERNEL_LMA;

    .boot : {
        *(.boot)
    }

    . += KERNEL_VMA;

    .init ALIGN(16) : AT(ADDR(.init) - KERNEL_VMA) {
        *(.init.text)
        *(.init.data)
    }

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(16) (NOLOAD) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        *(.allot)

        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
    }
}