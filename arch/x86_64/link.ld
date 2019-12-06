OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_boot)

KERNEL_LMA = 0x0000000000200000;
KERNEL_VMA = 0xFFFFFFFF80000000 + KERNEL_LMA;

SECTIONS {
    . = KERNEL_VMA;

    .boot KERNEL_LMA : AT(ADDR(.boot)) {
        *(.boot)
    }

    .init ALIGN(16) : AT(ADDR(.init) - KERNEL_VMA) {
        *(.init.text)
        *(.init.data)

        . = ALIGN(4K);
    }

    .text : AT(ADDR(.text) - KERNEL_VMA) {
        *(.text)
        *(.text.*)

        . = ALIGN(4K);
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata)
        *(.rodata.*)

        . = ALIGN(4K);
    }

    .data : AT(ADDR(.data) - KERNEL_VMA) {
        *(.data)
        *(.data.*)

        . = ALIGN(4K);
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) {
        *(.eh_frame)

        . = ALIGN(4K);
    }

    .bss ALIGN(16) (NOLOAD) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        *(.allot)
    }

    /DISCARD/ : {
        *(.comment)
    }
}