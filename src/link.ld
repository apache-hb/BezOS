OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = BEGIN;

    .boot : AT(ADDR(.boot) - BEGIN) {
        *(.real*)
        *(.prot*)
        *(.long*)
        . = ALIGN(512);
    }

    BOOT_SECTORS = (. - BEGIN) / 512;
    BOOT_END = .;

    . += VMA;

    .kern : AT(ADDR(.kern) - VMA - BEGIN) {
        *(.text*)
        *(.data*)
        *(.rodata*)
    }


    BSS_BEGIN = ALIGN(4);
    .ignore : { *(.bss*) }
    BSS_END = ALIGN(4);


    /DISCARD/ : {
        *(COMMON)
        *(.comment)
        *(.eh_frame)
    }
}