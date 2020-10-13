OUTPUT_FORMAT(binary)

VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = VMA;

    .text : AT(ADDR(.text) - VMA) {
        TEXT_BEGIN = .;
        *(.kmain*)
        *(.text*)
        TEXT_END = .;
    }

    .data BLOCK(4K) : AT(ADDR(.data) - VMA) {
        DATA_BEGIN = .;
        *(.data*)
        DATA_END = .;
    }

    .rodata BLOCK(4K) : AT(ADDR(.rodata) - VMA) {
        RODATA_BEGIN = .;
        *(.rodata*)
        RODATA_END = .;
    }

    .bss BLOCK(4K) : AT(ADDR(.bss) - VMA) {
        BSS_BEGIN = .;
        *(.bss*)
        *(COMMON)
        BSS_END = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}