#include <posix/setjmp.h>

#include <posix/assert.h>
#include "private.hpp"

int setjmp(jmp_buf env) noexcept {
    Unimplemented();
    assert(false && "setjmp not implemented");
    __builtin_unreachable();
}

void longjmp(jmp_buf, int) noexcept {
    Unimplemented();
    assert(false && "longjmp not implemented");
    __builtin_unreachable();
}

int sigsetjmp(sigjmp_buf, int) noexcept {
    Unimplemented();
    assert(false && "sigsetjmp not implemented");
    __builtin_unreachable();
}

void siglongjmp(sigjmp_buf, int) noexcept {
    Unimplemented();
    assert(false && "siglongjmp not implemented");
    __builtin_unreachable();
}
