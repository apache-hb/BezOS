#include "arch/x86_64/asm.h"

.extern KmSystemDispatchRoutine
.global KmSystemEntry

.global KmSystemCallStackTlsOffset

.section .bss

KmSystemCallStackTlsOffset:
    .quad 0

.section .text
.cfi_sections .debug_frame

/** syscall entry point
 * @param rax syscall number
 * @param rdi arg0
 * @param rsi arg1
 * @param rdx arg2
 * @param r8  arg3
 *
 * @param rcx return address
 * @param r11 eflags
 *
 * @clobber r15, r9
 * @preserve rbp, rsp
 *
 * @return rax - result
 */
PROC_SIMPLE(KmSystemEntry)
    .cfi_signal_frame
    .cfi_def_cfa %rsp, 0
    .cfi_register %rip, %rcx
    .cfi_register %rflags, %r11

    /* FMASK is expected to mask out interrupts before we get here
       We need to re-enable them so that the kernel remains pre-emptible.
       But NOT BEFORE SWAPGS, the interrupt handlers expect gsbase to be the kernel gs
       when an interrupt arrives with cpl=0. If an interrupt happened before this swapgs
       that precondition would be violated, causing an incorrect swapgs and a crash */
    swapgs

    movq KmSystemCallStackTlsOffset, %r9
    movq %gs:(%r9), %r15

    xchg %r15, %rsp

    sti

    /* pushed first to satisfy the system-v ABI */
    PUSHQ_CFI_REG_ALIAS(%rcx, %rip)
    PUSHQ_CFI_REG(%rbp)

    movq %rsp, %rbp

    PUSHQ_CFI_REG_ALIAS(%r15, %rsp) /* userStack */
    PUSHQ_CFI_REG(%rax) /* function */
    PUSHQ_CFI_REG(%rdi) /* arg0 */
    PUSHQ_CFI_REG(%rsi) /* arg1 */
    PUSHQ_CFI_REG(%rdx) /* arg2 */
    PUSHQ_CFI_REG(%r8) /* arg3 */
    PUSHQ_CFI_REG(%r11) /* eflags */

    /* Save general registers */
    PUSHQ_CFI_REG(%rbx)
    PUSHQ_CFI_REG(%r10)
    PUSHQ_CFI_REG(%r12)
    PUSHQ_CFI_REG(%r13)
    PUSHQ_CFI_REG(%r14)
    /* Call the dispatch routine, passing the context struct */
    mov %rsp, %rdi
    call KmSystemDispatchRoutine
    /* Routine returns into rax:rdx, so no need to move it */

    /* Restore general registers */
    POPQ_CFI_REG(%r14)
    POPQ_CFI_REG(%r13)
    POPQ_CFI_REG(%r12)
    POPQ_CFI_REG(%r10)
    POPQ_CFI_REG(%rbx)
    /* set up registers for sysret */

    POPQ_CFI_REG(%r11) /* eflags */
    or $0x200, %r11 /* set the IF flag before returning control */

    /* pop function and arg0-arg3 */
    add $(5 * 8), %rsp
    .cfi_adjust_cfa_offset -5 * 8

    /* Restore the user stack */
    POPQ_CFI_REG(%r15)

    POPQ_CFI_REG(%rbp)
    POPQ_CFI_REG(%rcx)

    /* Clear the interrupt flag before swapgs, otherwise an interrupt
       could observe the violated precondition. */
    cli

    movq %r15, %rsp

    swapgs

    sysretq
ENDP(KmSystemEntry)
