#include "arch/intrin.hpp"
#include "log.hpp"
#include "panic.hpp"

[[noreturn]]
void KmHalt(void) {
    for (;;) {
        __cli();
        __halt();
    }
}

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    KmDebugMessage("[BUG] ", file, ":", line, "\n");
    KmDebugMessage("[BUG] ", message, "\n");
    KmHalt();
}
