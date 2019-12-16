OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

KERNEL_LMA = 0x0000000000100000;
KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = KERNEL_LMA + KERNEL_VMA;

    .text ALIGN(4K) : {
        text_front = . - KERNEL_VMA;

        KEEP(*(.text*))
        KEEP(*(.multiboot))
        
        text_back = . - KERNEL_VMA;
    }

    .data ALIGN(4K) : {
        data_front = . - KERNEL_VMA;
        
        KEEP(*(.data*))
        KEEP(*(.rodata))
        
        data_back = . - KERNEL_VMA;
    }

    .bss ALIGN(4K) : {
        bss_front = . - KERNEL_VMA;
        
        KEEP(*(.bss*))
        
        bss_back = . - KERNEL_VMA;
    }
}