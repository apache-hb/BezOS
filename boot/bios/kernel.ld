OUTPUT_FORMAT(binary)

VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = VMA;
    .text ALIGN(4K) : { *(.text*) }
    .data ALIGN(4K) : { *(.data*) }
    .rodata ALIGN(4K) : { *(.rodata*) }
}