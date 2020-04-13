OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = BEGIN;

    .boot : AT(ADDR(.boot) - BEGIN) {
        *(.real*)
        *(.prot*)
        *(.long*)
    }

    . += VMA;

    .kern : AT(ADDR(.kern) - VMA - BEGIN) {
        *(.text*)
        *(.data*)
        *(.rodata*)
    }

    .ignore : { *(.bss*) }

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
        *(.eh_frame)
    }
}