OUTPUT_FORMAT(binary)

KERNEL_VMA = 0x7C00;

SECTIONS {
    . = KERNEL_VMA;
    .bootloader : { 
        *(.bootloader)
    }
    
    .protected : { 
        *(.protected) 
    }
    
    .text : { 
        *(.text) 
    }

    .data : {
        *(.rodata)
        *(.data)

        /* we need these to figure out how much memory we need to zero */
        BSS_FRONT = .;
        *(.bss)
        BSS_BACK = .;
    }

    /* align the kernel to the next page */
    KERNEL_END = ALIGN(4K);
    KERNEL_SECTORS = KERNEL_END / 512;

    /DISCARD/ : {
        *(COMMON)
        *(.comment)
        *(.eh_frame)
    }
}