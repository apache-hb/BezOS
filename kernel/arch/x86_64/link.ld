ENTRY(_boot)

SECTIONS {
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}