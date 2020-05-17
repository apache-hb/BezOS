OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFFF0000000;

SECTIONS {
    . = BEGIN;
    .boot : { 
        *(.boot16*) 
        *(.boot32*)
        *(.boot64*)
    }

    . += VMA;

    HIGHER_START = . - VMA;

    .kernel : AT(ADDR(.kernel) - VMA) {
        *(.text)
        *(.data)
        *(.rodata)
    }

    HIGHER_END = . - VMA;


    IGNORE_START = . - VMA;
    .ignore : AT(ADDR(.ignore) - VMA) {
        *(.bss)
    }
    IGNORE_END = . - VMA;

    SECTORS = (. - VMA - BEGIN) / 512;


    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(COMMON)
    }
}