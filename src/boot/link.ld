OUTPUT_FORMAT(elf64-x86-64)

BOOT_ADDR = 0x7C00;
VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = BOOT_ADDR;

    .bootload : {
        *(.boot*)
        *(.protected*)
        *(.long*)
    }
    . = ALIGN(512);

    BOOT_END = . - BOOT_ADDR;
    BOOT_SECTORS = (BOOT_END / 512) - 1;


    . = VMA;


    KERN_FRONT = .;
    .text : AT(ADDR(.text) - VMA) {
        *(.text*)
    }
    .data : AT(ADDR(.data) - VMA) {
        *(.data*)
        *(.rodata)
    }
    . = ALIGN(512);
    KERN_BACK = .;
    

    KERN_SECTORS = ((KERN_BACK - KERN_FRONT) / 512) - 1;

    SECTORS = BOOT_SECTORS + KERN_SECTORS;    

    BSS_FRONT = ALIGN(4);
    .ignore : { *(.bss*) }
    BSS_BACK = ALIGN(4);

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
    }
}