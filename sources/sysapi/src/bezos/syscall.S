#include "bezos/arch/x86_64/asm.h"

PROC(OsSystemCall)
    movq %rdi, %rax /* syscall number */
    movq %rsi, %rdi /* arg0 */
    movq %rdx, %rsi /* arg1 */
    movq %rcx, %rdx /* arg2 */
    /* mov %r8, %r8 */ /* arg3 */
    /* mov %r9, %r9 */ /* arg4 */
    syscall
    /* rax:rdi */ /* return value */
    retq
ENDP(OsSystemCall)
