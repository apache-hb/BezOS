OUTPUT_FORMAT(binary)

SECTIONS {
    . = 0x7C00;
    .real : { *(.real*) }

    SECTORS = (. - 0x7C00) / 512 - 1;
}
