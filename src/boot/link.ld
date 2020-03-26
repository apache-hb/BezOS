OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    . = 0x7C00;

    .kernel : {
        *(.boot*)
        *(.protected*)
        *(.long*)
        *(.text*)
        *(.data*)
        *(.rodata*)
    }
    
    . = ALIGN(512);
    KERNEL_END = . - 0x7C00;
    KERNEL_SECTORS = (KERNEL_END / 512) - 1;
    
    BSS_FRONT = ALIGN(4);
    .ignore : { *(.bss*) }
    BSS_BACK = ALIGN(4);

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
    }
}