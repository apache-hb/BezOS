OUTPUT_FORMAT(binary)

VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - VMA) {
        TEXT_BEGIN = .;
        *(.text*)
        TEXT_END = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - VMA) {
        DATA_BEGIN = .;
        *(.data*)
        DATA_END = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - VMA) {
        RODATA_BEGIN = .;
        *(.rodata*)
        RODATA_END = .;
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - VMA) {
        BSS_BEGIN = .;
        *(.bss*)
        BSS_END = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(COMMON)
    }
}