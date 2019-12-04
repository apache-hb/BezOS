.code32

.section bootstrap_text, "ax"
.global _start32
.type _start32, @function
_start32:
    cld

.code64
    call _start64
.size _start32, . - _start32

.section .text
.type _start64, @function
_start64:
    call kernel_main

.section .bss
.align 4096
.skip 4096
kernel_stack:
