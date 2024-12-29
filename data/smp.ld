OUTPUT_FORMAT(binary)

SECTIONS
{
    .text 0x8000 : {
        *(.smp.startup*)
    }
}
