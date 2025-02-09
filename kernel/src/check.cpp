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
    stdx::StringView fn(where.function_name(), std::char_traits<char>::length(where.function_name()));
    stdx::StringView file(where.file_name(), std::char_traits<char>::length(where.file_name()));
    KmDebugMessage("[BUG] ", fn, " (", file, ":", where.line(), ")\n");
    KmHalt();
}
