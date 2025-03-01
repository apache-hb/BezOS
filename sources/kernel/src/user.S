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
 * @clobber rax, rdi, rsi, rdx, r11, rcx, rsp, rbp, r15, r9
 *
 * @return rax - result
 */
KmSystemEntry:
    /* FMASK is expected to mask out interrupts before we get here */
    /* We need to re-enable them so that the kernel remains pre-emptible. */
    /* But NOT BEFORE SWAPGS, the interrupt handlers expect gsbase to be the kernel gs */
    /* when an interrupt arrives with cpl=0. If an interrupt happened before this swapgs */
    /* that precondition would be violated, causing an incorrect swapgs and a crash */
    swapgs
    sti

    movq %rsp, %r15
    movq KmSystemCallStackTlsOffset, %r9

    movq %gs:(%r9), %rsp

    /* Construct a SystemCallRegisterSet */

    pushq %r15 /* userStack */
    pushq %rax /* function */
    pushq %rdi /* arg0 */
    pushq %rsi /* arg1 */
    pushq %rdx /* arg2 */
    pushq %r8  /* arg3 */
    pushq %r11 /* eflags */
    pushq %rcx /* userReturnAddress */

    /* Save general registers */
    pushq %rbp
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
    popq %rbp

    /* set up registers for sysret */

    popq %rcx /* userReturnAddress */
    popq %r11 /* eflags */
    or $0x200, %r11 /* set the IF flag before returning control */

    /* pop the rest of the context we dont need to preserve */
    add $(5 * 8), %rsp

    /* Restore the user stack */
    popq %rsp

    /* Clear the interrupt flag before swapgs, otherwise an interrupt */
    /* could observe the violated precondition. */
    cli

    swapgs

    sysretq
