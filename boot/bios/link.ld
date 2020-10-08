OUTPUT_FORMAT(binary)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFF80000000;

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

    BOOT_END = .;

    BOOT_SECTORS = (BOOT_END - BOOT_BEGIN) / 512;
    PT_ADDR = ALIGN(0x1000);

    KERNEL_BEGIN = .;
    .kernel ALIGN(512) : { *(.kernel*) }
    KERNEL_END = .;

    KERNEL_SECTORS = (KERNEL_END - KERNEL_BEGIN) / 512;

    /* TODO: int 13h can only read 64k at a time, 
     * when the kernel gets bigger than that
     * we'll need to update stuff here to 
     * accomidate
     */

    .bss ALIGN(0x1000) : AT(ADDR(.bss) - VMA) { 
        BSS_BEGIN = .;
        *(.bss)
        BSS_END = .; 
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(COMMON)
    }
}