OUTPUT_FORMAT(binary)

BEGIN = 0x7C00;

SECTIONS {
    . = BEGIN;

    BOOT_BEGIN = .;

    .boot : { *(.boot*) }

    .text : {
        TEXT_BEGIN = .;
        *(.text*)
        TEXT_END = .;
    }

    .data : {
        DATA_BEGIN = .;
        *(.data*)
        DATA_END = .;
    }

    .rodata : {
        RODATA_BEGIN = .;
        *(.rodata*)
        RODATA_END = .;
    }

    .bss : { 
        BSS_BEGIN = .;
        *(.bss)
        *(COMMON)
        BSS_END = .; 
    }

    BOOT_END = .;

    PT_ADDR = ALIGN(0x1000);

    KERNEL_BEGIN = .;
    .kernel ALIGN(512) : { *(.kernel*) }
    KERNEL_END = .;

    BOOT_SECTORS = ((BOOT_END - BOOT_BEGIN) / 512) + 1;
    KERNEL_SECTORS = ((KERNEL_END - KERNEL_BEGIN) / 512) + 1;

    /* TODO: int 13h can only read 64k at a time, 
     * when the kernel gets bigger than that
     * we'll need to update stuff here to 
     * accomidate
     */

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}