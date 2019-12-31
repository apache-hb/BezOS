OUTPUT_FORMAT(binary)

KERNEL_VMA = 0x7C00;

SECTIONS {
    . = KERNEL_VMA;
    .bootloader : { *(.bootloader) }
    .protected : { *(.protected) }
    .text : { *(.text) }
    .data : {
        *(.rodata)
        *(.data)
        *(.bss)
    }

    KERNEL_END = .;

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
        *(.eh_frame)
    }
}