OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    .boot : {
        *(.boot)
    }
    . = ALIGN(512);
    .protected : {
        *(.protected)
    }
    .long : {
        *(.long)
    }

    .text : {
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