OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    
    .real : {
        *(.real)
        *(.boot)
    }

    . = ALIGN(4K);

    .text : {
        /* execute only data */
        TEXT_START = .;
    
        *(.prot)
        *(.long)
        *(.text)
        
        TEXT_END = .;
    }

    /* page align each section that has different permissions */
    . = ALIGN(4K);

    .data : {
        /* read/write data */
        DATA_START = .;
        
        *(.data)
        
        DATA_END = .;
    }

    . = ALIGN(4K);

    .rodata : {
        /* read only data */
        RODATA_START = .;
        
        *(.rodata)
        
        RODATA_END = .;
    }

    .bss : {
        /* read/write zeros */
        BSS_START = .;
        *(.bss)
        BSS_END = .;
    }

    /* align to next sector */
    . = ALIGN(512);

    /* the size in bytes of the entire kernel */
    KERNEL_END = . - 0x7C00;
    KERNEL_SECTORS = KERNEL_END / 512;
}