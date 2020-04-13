OUTPUT_FORMAT(elf64-x86-64)

BEGIN = 0x7C00;

SECTIONS {
    . = BEGIN;

    .boot : {
        *(.real*)
        *(.prot*)
        *(.long*)
    }

    .kern : {
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