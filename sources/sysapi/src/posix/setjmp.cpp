#include <posix/setjmp.h>

#include <posix/assert.h>

int setjmp(jmp_buf env) noexcept {
    assert(false && "setjmp not implemented");
    __builtin_unreachable();
}

void longjmp(jmp_buf, int) noexcept {
    assert(false && "longjmp not implemented");
    __builtin_unreachable();
}

int sigsetjmp(sigjmp_buf, int) noexcept {
    assert(false && "sigsetjmp not implemented");
    __builtin_unreachable();
}

void siglongjmp(sigjmp_buf, int) noexcept {
    assert(false && "siglongjmp not implemented");
    __builtin_unreachable();
}
