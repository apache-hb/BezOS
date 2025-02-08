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

[[noreturn]]
void KmIdle(void) {
    for (;;) {
        __sti();
        __halt();
    }
}

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    KmDebugMessage("[BUG] ", file, ":", line, "\n");
    KmDebugMessage("[BUG] ", message, "\n");
    KmHalt();
}

void km::BugCheck(stdx::StringView message, std::source_location where) {
    KmDebugMessage("[BUG] Assertion failed '", message, "'\n");
    KmDebugMessage("[BUG] ", where.function_name(), " (", where.file_name(), ":", where.line(), ")\n");
    KmHalt();
}
