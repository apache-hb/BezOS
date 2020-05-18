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

    BOOTSECTORS = (. - BEGIN) / 512;
    BOOTEND = ALIGN(4) - BEGIN;

    . += VMA;

    .text : AT(ADDR(.text) - VMA) { *(.text) }
    .data : AT(ADDR(.data) - VMA) { *(.data) }
    .rodata : AT(ADDR(.rodata) - VMA) { *(.rodata) }


    .ignore : AT(ADDR(.ignore) - VMA) { *(.bss) }


    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(COMMON)
    }
}