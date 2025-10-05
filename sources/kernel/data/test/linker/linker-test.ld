/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* We want the symbol kmain to be our entry point */
ENTRY(LimineMain)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    boot_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    /* We want to be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    . = 0xffffffff80000000;

    __kernel_start = .;

    /* Define a section to contain the Limine requests and assign it to its own PHDR */
    .boot_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :boot_requests

    /* Move to the next memory page for .text */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        __text_start = .;
        *(.text .text.*)
        __text_end = .;
    } :text

    /* Move to the next memory page for .rodata */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        __rodata_start = .;
        *(.rodata .rodata.*)

        __rodata_end = .;
    } :rodata

    /* Move to the next memory page for .data */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        __data_start = .;
        *(.data .data.*)

        . = ALIGN(CONSTANT(MAXPAGESIZE));
        __test_cases_start = .;
        KEEP(*(.testing))
        __test_cases_end = .;

        __data_end = .;
    } :data

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
        __data_end = .;
    } :data

    __kernel_end = .;

    /* Move to the next memory page for .cpudata */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .cpudata (NOLOAD) : {
        __cpudata_start = .;
        *(.cpuroot .cpuroot.*)
        . = ALIGN(8);
        *(.cpudata .cpudata.*)
        __cpudata_end = .;
    }

    /* Discard .note.* and .eh_frame* since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
