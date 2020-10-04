OUTPUT_FORMAT(binary)

BEGIN = 0x7C00;

SECTIONS {
    . = BEGIN;
    .boot : { *(.boot*) }
    .text : { *(.text*) }
    .data : { *(.data*) }
    .rodata : { *(.rodata*) }
}