ENTRY(_start32)

SECTIONS {
    . = 0x100000;

    bootstrap_begin = .;

    bootstrap_text : {
        *(multiboot)
        *(multiboot_text)
    }

    bootstrap_rodata : {
        *(bootstrap_rodata)
    }

    bootstrap_bss ALIGN(4K) : {
        *(bootstrap_bss)
    }

    bootstrap_end = ALIGN(4K);
    num_bootstrap_pages = (bootstrap_end - bootstrap_begin) / 0x1000;

    . += 0xFFFFFFFF80000000;

    kernel_virtual_begin = ALIGN(4K);
    kernel_physical_begin = kernel_virtual_begin - 0xFFFFFFFF80000000;

    .text : AT(ADDR(.text) - 0xFFFFFFFF80000000) {
        *(.text*)
    }

    kernel_exec_end = ALIGN(4K);
    num_exec_pages = (kernel_exec_end - kernel_virtual_begin) / 0x1000;

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFFFF80000000) {
        *(.rodata*)
    }

    kernel_readonly_end = ALIGN(4K);
    num_readonly_pages = (kernel_readonly_end - kernel_virtual_begin) / 0x1000;

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFFFF80000000) {
        *(.bss)
    }

    kernel_virtual_end = ALIGN(4K);
    kernel_physical_end = kernel_virtual_end - 0xFFFFFFFF80000000;
    num_write_pages = (kernel_virtual_end - kernel_readonly_end) / 0x1000;
}