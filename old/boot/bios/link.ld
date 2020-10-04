OUTPUT_FORMAT(binary)

BEGIN = 0x7C00;
VMA = 0xFFFFFFFF80000000;

SECTIONS {
    . = BEGIN;

    .boot : {
        *(.boot16*)
        *(.boot32*)
        *(.boot64*)
        *(.bootc*)
    }

    BOOT_SECTORS = (. - BEGIN) / 512;
    PT_ADDR = ALIGN(0x1000);

    . += VMA;

    KERNEL_BEGIN = ALIGN(0x1000);
    .text ALIGN(0x1000) : AT(ADDR(.text) - VMA) {
        TEXT_BEGIN = .;
        *(.text*)
        TEXT_END = .;
    }

    .data ALIGN(0x1000) : AT(ADDR(.data) - VMA) {
        DATA_BEGIN = .;
        *(.data*)
        DATA_END = .;
    }

    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - VMA) {
        RODATA_BEGIN = .;
        *(.rodata*)
        RODATA_END = .;
    }

    SECTORS = (. - VMA) / 512;

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

    KERNEL_END = ALIGN(0x1000);
    KERNEL_PAGES = ((KERNEL_END - KERNEL_BEGIN) / 0x1000) + 1;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(COMMON)
    }
}