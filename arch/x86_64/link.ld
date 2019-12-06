OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_boot)

KERNEL_LMA = 0x0000000000100000;
KERNEL_VMA = 0xFFFFFFFF80100000;

SECTIONS {
    . = KERNEL_LMA;

    .boot KERNEL_LMA : AT(ADDR(.boot)) {
        *(.boot)
        . = ALIGN(4K);
    }

    .init ALIGN(16) : AT(ADDR(.init) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.init.text)
        *(.init.data)
    }

    .text ALIGN(4K) : AT(ADDR(.text) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.data)
        *(.data.*)
    }

    .eh_frame ALIGN(4K) : AT(ADDR(.eh_frame) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.eh_frame_hdr)
        *(.eh_frame)
    }

    .bss ALIGN(16) (NOLOAD) : AT(ADDR(.bss) - (KERNEL_VMA + KERNEL_LMA)) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        *(.allot)
    }

    /DISCARD/ : {
        *(.comment)
    }
}