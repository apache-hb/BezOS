OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    .boot : {
        *(.boot)
    }
    .protected : {
        *(.protected)
    }

    .text : {
        *(.text.front)
        *(.text)
    }
    .data : {
        *(.data)
        *(.rodata)
        *(.bss)
    }

    KERNEL_END = ALIGN(512) - 0x7C00;
    KERNEL_SECTORS = (KERNEL_END / 512) - 1;

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
    }
}