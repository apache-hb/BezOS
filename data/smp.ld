OUTPUT_FORMAT(binary)

PHDRS
{
    data PT_LOAD;
}

SECTIONS
{
    .data : {
        . = 0x8000;
        *(.smp.startup*)
    } :data
}
