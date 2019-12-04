#include "multiboot.h"
#include "arch.h"

.code32
.section .multiboot
    
    .align 4
    multiboot:
        .long MULTIBOOT_HEADER_MAGIC ## magic
        .long MULTIBOOT_HEADER_FLAGS ## flags
        .long ~(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) ## checksum

    .global _start32
    _start32:
        cli