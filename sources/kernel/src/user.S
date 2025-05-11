#include "arch/x86_64/asm.h"

.extern KmSystemDispatchRoutine

.global KmSystemEntry

.global KmSystemCallStackTlsOffset

.section .bss

KmSystemCallStackTlsOffset:
    .quad 0

.section .text

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
    pushq %rcx /* rcx is the return address */
    pushq %rbp
    movq %rsp, %rbp

    pushq %r15 /* userStack */
    pushq %rax /* function */
    pushq %rdi /* arg0 */
    pushq %rsi /* arg1 */
    pushq %rdx /* arg2 */
    pushq %r8  /* arg3 */
    pushq %r11 /* eflags */

    /* Save general registers */
    pushq %rbx
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    /* Call the dispatch routine, passing the context struct */
    mov %rsp, %rdi
    call KmSystemDispatchRoutine
    /* Routine returns into rax:rdx, so no need to move it */

    /* Restore general registers */
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %rbx
    /* set up registers for sysret */

    popq %r11 /* eflags */
    or $0x200, %r11 /* set the IF flag before returning control */

    /* pop function and arg0-arg3 */
    add $(5 * 8), %rsp

    /* Restore the user stack */
    popq %r15

    popq %rbp
    popq %rcx

    movq %r15, %rsp

    /* Clear the interrupt flag before swapgs, otherwise an interrupt
       could observe the violated precondition. */
    cli

    swapgs

    sysretq
ENDP(KmSystemEntry)
