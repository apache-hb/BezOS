/* Enter ring 3, jumping to a supplied address. */
/* rdi: address to jump to */
/* rsi: eflags */

.global KmEnterRing3

KmEnterRing3:
    mov %rdi, %rcx
    mov %rsi, %r11
    sysretq
