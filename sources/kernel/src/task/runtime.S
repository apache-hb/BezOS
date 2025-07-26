#include "arch/x86_64/asm.h"

PROC(__x86_64_idle)
    sti
    hlt
    jmp __x86_64_idle
ENDP(__x86_64_idle)
