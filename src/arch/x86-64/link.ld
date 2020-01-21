OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    
    .real : {
        *(.real)
        *(.boot)
<<<<<<< HEAD
        *(.prot)
=======
    }
    .protected : {
        *(.protected)
    }
    .long : {
>>>>>>> parent of e4cb183... add some more stuff
        *(.long)
    }

    .text : ALIGN(4K) {
        /* execute only data */
        TEXT_START = .;

        *(.text)
        
        TEXT_END = .;
    }

    .data : ALIGN(4K) {
        /* read/write data */
        DATA_START = .;
        
        *(.data)
        
        DATA_END = .;
    }

    .rodata : ALIGN(4K) {
        /* read only data */
        RODATA_START = .;
        
        *(.rodata)
        
        RODATA_END = .;
    }

    .bss : ALIGN(4K) {
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