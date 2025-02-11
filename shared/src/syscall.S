.global OsSystemCall

OsSystemCall:
    mov %rdi, %rax /* syscall number */
    mov %rsi, %rdi /* arg0 */
    mov %rdx, %rsi /* arg1 */
    mov %rcx, %rdx /* arg2 */
    /* mov %r8, %r8 */ /* arg3 */
    /* mov %r9, %r9 */ /* arg4 */
    syscall
    /* rax:rdi */ /* return value */
    ret
