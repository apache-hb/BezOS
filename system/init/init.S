.global OsSystemCall

/** Invoke a syscall from userspace
 * @param rdi syscall number
 * @param rsi arg1
 * @param rdx arg2
 * @param rcx arg3
 * @param r8 arg4
 *
 * @return rax - result
 */
OsSystemCall:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    /* r8 is already setup by the caller */
    syscall
    ret
