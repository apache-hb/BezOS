OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = BEGIN;

    .boot : {
        BOOT_BEGIN = .;
        *(.real*)
        *(.prot*)
        *(.long*)
        BOOT_END = .;
    }

    . += VMA;

    .kern : AT(ADDR(.kern) - VMA) {
        KERN_BEGIN = ALIGN(4);
        *(.text*)
        *(.data*)
        *(.rodata*)
        KERN_END = ALIGN(512);
    }

    BSS_BEGIN = ALIGN(8);
    .ignore : { *(.bss*) }
    BSS_END = ALIGN(8);




    /* public variables */
    BOOT_ADDR = BOOT_BEGIN;
    BOOT_SIZE = BOOT_END - BOOT_BEGIN;

    KERN_ADDR = KERN_BEGIN;
    KERN_SIZE = KERN_END - KERN_BEGIN;

    BSS_ADDR = BSS_BEGIN;
    BSS_SIZE = BSS_END - BSS_BEGIN;

    SECTORS = (BOOT_SIZE + KERN_SIZE) / 512 - 1;



    /DISCARD/ : {
        *(COMMON)
        *(.comment)
    }
}