OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;

SECTIONS {
    . = BEGIN;

    .boot : AT(ADDR(.boot) - BEGIN) {
        *(.real*)
        *(.prot*)
        *(.long*)
        . = ALIGN(512);
    }

    .kern : {
        *(.text*)
        *(.data*)
        *(.rodata*)
    }

    BSS_ADDR = ALIGN(8);
    .ignore : { *(.bss*) }
    BSS_SIZE = ALIGN(8) - BSS_ADDR;

    BSS_DWORDS = BSS_SIZE / 4;

    . = ALIGN(8);

    END = . - BEGIN;
    SECTORS = (. - BEGIN) / 512;

    /* physical address and size of first stage loader */
    KERN_ADDR = ADDR(.kern) - BEGIN;
    KERN_SIZE = . - BEGIN;

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
        *(.eh_frame)
    }
}