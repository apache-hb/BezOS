OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x100000;
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

ENTRY(bootstrap)

SECTIONS {
    . = KERNEL_VMA;

    _start = .;

    .text : AT(ADDR(.text) - KERNEL_VMA) {
        _code = .;
        *(.multiboot)
        *(.text*)
        *(.gnu.linkonce.t*)
        . = ALIGN(4K);
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        _rodata = .;
        *(.rodata*)
        *(.gnu.linkonce.r*)

        . = ALIGN(8);
        __CTOR_LIST__ = .;
        *(.ctors)
        __CTRO_END__ = .;

        . = ALIGN(8);
        __DTOR_LIST__ = .;
        *(.dtors)
        __DTOR_END__ = .;

        . = ALIGN(8);
        __INIT_LIST = .;
        KEEP (*(.init_array))
        __INIT_END__ = .;

        . = ALIGN(8);
        __FINI_LIST__ = .;
        KEEP (*(.fini_array))
        __FINI_END__ = .;

        . = ALIGN(4K);
    }

    .data : AT(ADDR(.data) - KERNEL_VMA) {
        _data = .;
        *(.data)
        *(.gnu.linkonce.d*)
        . = ALIGN(4K);
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) {
        _eh_frame = .;
        *(.eh_frame)
        *(.eh_frame_hdr)
        . = ALIGN(4K);
    }

    _edata = .;

    .bss : AT(ADDR(.bss) - KERNEL_VMA) {
        _bss = .;
        *(.bss*)
        *(.gnu.linkonce.b*)
        *(COMMON)
        . = ALIGN(4K);
    }

    _end = .;

    /DISCARD/ : {
        *(.comment)
        *(note.*)
    }
}