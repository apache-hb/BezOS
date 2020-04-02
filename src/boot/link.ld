OUTPUT_FORMAT(elf64-x86-64)

VIRT_ADDRESS = 0xFFFFFFFF80000000;

SECTIONS {
    . = 0x7C00;

    .bootload : {
        *(.boot*)
        *(.protected*)
        *(.long*)
    }

    BOOT_END = . - 0x7C00;
    BOOT_SECTORS = (BOOT_END / 512) - 1;

    .kernel : {
        FILL(0x0);
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