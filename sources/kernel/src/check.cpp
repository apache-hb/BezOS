#include "arch/intrin.hpp"
#include "log.hpp"
#include "panic.hpp"
#include "setup.hpp"

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfunction-effects" // BugCheck isn't actually nonblocking

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) noexcept [[clang::nonblocking]]  {
    KmDebugMessageUnlocked("[BUG] ", file, ":", line, "\n");
    KmDebugMessageUnlocked("[BUG] ", message, "\n");
    km::DumpCurrentStack();
    KmHalt();
}

void km::BugCheck(stdx::StringView message, std::source_location where) noexcept [[clang::nonblocking]]  {
    KmDebugMessageUnlocked("[BUG] Assertion failed '", message, "'\n");
    stdx::StringView fn(where.function_name(), std::char_traits<char>::length(where.function_name()));
    stdx::StringView file(where.file_name(), std::char_traits<char>::length(where.file_name()));
    KmDebugMessageUnlocked("[BUG] ", fn, " (", file, ":", where.line(), ")\n");
    DumpCurrentStack();
    KmHalt();
}

#pragma clang diagnostic pop
