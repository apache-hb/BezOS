#include "bezos/arch/x86_64/asm.h"

PROC(OsSystemCall)
    /* prelude */
    pushq %rbp
    movq %rsp, %rbp

    /* syscall clobbers r15 */
    pushq %r15

    movq %rdi, %rax /* syscall number */
    movq %rsi, %rdi /* arg0 */
    movq %rdx, %rsi /* arg1 */
    movq %rcx, %rdx /* arg2 */
    /* mov %r8, %r8 */ /* arg3 */
    /* mov %r9, %r9 */ /* arg4 */
    syscall
    /* rax:rdx contains the result */

    popq %r15

    popq %rbp
    /* epilogue */
    retq
ENDP(OsSystemCall)
