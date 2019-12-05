OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_boot)

KERNEL_LMA = 0x0000000000200000;
KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = KERNEL_VMA;

    _start = .;

    .text : AT(ADDR(.text) - KERNEL_VMA) {
        _text = .;
        *(.multiboot)
        *(.text)

        . = ALIGN(4K);
    }

    .data : AT(ADDR(.data) - KERNEL_VMA) {
        _data = .;
        *(.data)

        . = ALIGN(4K);
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) {
        _eh_frame = .;
        *(.eh_frame)

        . = ALIGN(4K);
    }

    _edata = .;

    .bss : AT(ADDR(.bss) - KERNEL_VMA) {
        _bss = .;
        *(.bss)
        *(COMMON)

        . = ALIGN(4K);
    }

    _end = .;

    /DISCARD/ : {
        *(.comment)
    }
}