ENTRY(_boot)

SECTIONS {
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(.bss)
    }

    /DISCARD/ : {
        *(.comment)
        *(COMMON)
        *(.eh_frame)
    }
}