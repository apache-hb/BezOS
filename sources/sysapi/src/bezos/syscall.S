#include "bezos/arch/x86_64/asm.h"

.section .text
.cfi_sections .debug_frame

PROC(OsSystemCall)
    /* prelude */
    pushq %rbp
    .cfi_adjust_cfa_offset 8
    .cfi_rel_offset %rbp, 0

    movq %rsp, %rbp
    .cfi_def_cfa_register %rbp

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
    .cfi_restore %rbp
    .cfi_def_cfa %rsp, 8

    /* epilogue */
    retq
ENDP(OsSystemCall)
