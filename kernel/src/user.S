.extern KmSystemDispatchRoutine

.global KmEnterUserMode
.global KmSystemEntry
.global OsSystemCall

.global KmSystemCallStackTlsOffset

.section .bss

KmSystemCallStackTlsOffset:
    .quad 0

.section .text

/* Enter ring 3, jumping to a supplied address. */
/* rdi: address to jump to */
/* rsi: eflags */
KmEnterUserMode:
    mov %rdi, %rcx
    mov %rsi, %r11

    swapgs

    sysretq

/** syscall entry point
 * @param rax syscall number
 * @param rdi arg1
 * @param rsi arg2
 * @param rdx arg3
 * @param r11 eflags - reserved by syscall instruction
 * @param rcx return address - reserved by syscall instruction
 *
 * @clobber rax, rdi, rsi, rdx, r11, rcx, rsp, rbp, r8, r9
 *
 * @return rax - result
 */
KmSystemEntry:
    swapgs

    movq %rsp, %r8
    movq KmSystemCallStackTlsOffset, %r9

    movq %gs:(%r9), %rsp

    /* Construct a SystemCallContext */

    pushq %r8  /* userStack */
    pushq %rax /* function */
    pushq %rdi /* arg0 */
    pushq %rsi /* arg1 */
    pushq %rdx /* arg2 */
    pushq %r11 /* eflags */
    pushq %rcx /* userReturnAddress */

    /* Save general registers */
    pushq %rbp
    pushq %rbx
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Call the dispatch routine, passing the context struct */
    mov %rsp, %rdi
    call KmSystemDispatchRoutine
    /* Routine returns into rax, so no need to move it */

    /* Restore general registers */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %rbx
    popq %rbp

    /* set up registers for sysret */

    popq %rcx /* userReturnAddress */
    popq %r11 /* eflags */

    /* pop the rest of the context we dont need to preserve */
    add $(4 * 8), %rsp

    /* Restore the user stack */
    popq %rsp

    swapgs

    sysretq

/** Invoke a syscall from userspace
 * @param rdi syscall number
 * @param rsi arg1
 * @param rdx arg2
 * @param rcx arg3
 *
 * @return rax - result
 */
OsSystemCall:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    syscall
    ret
