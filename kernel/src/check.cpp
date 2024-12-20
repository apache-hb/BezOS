#include "kernel.hpp"

#include "arch/intrin.hpp"

[[noreturn]]
void KmHalt(void) {
    for (;;) {
        __cli();
        __halt();
    }
}

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    KmDebugMessage("[BUGCHECK] ", file, ":", line, "\n");
    KmDebugMessage("[BUGCHECK] ", message, "\n");
    KmHalt();
}
