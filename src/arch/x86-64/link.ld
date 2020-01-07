OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    .boot : {
        *(.boot)
    }
    .protected : {
        *(.protected)
    }

    . = ALIGN(512);
    PROTECTED_END = . - 0x7C00;
    PROTECTED_SECTORS = (PROTECTED_END / 512) - 1;

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