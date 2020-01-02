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
        *(.text)
    }
    .data : {
        *(.data)
        *(.rodata)
        *(.bss)
    }

    KERNEL_END = ALIGN(0x1000);
    KERNEL_SECTORS = KERNEL_END / 512;

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
    }
}