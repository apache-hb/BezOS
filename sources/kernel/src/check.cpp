#include "panic.hpp"

#include "arch/intrin.hpp"
#include "logger/categories.hpp"
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
#pragma clang diagnostic ignored "-Wfunction-effects" // I want to be able to assert whenever

void km::BugCheck(stdx::StringView message, std::source_location where) noexcept [[clang::nonblocking]]  {
    InitLog.fatalf("Assertion failed '", message, "'");
    stdx::StringView fn(where.function_name(), std::char_traits<char>::length(where.function_name()));
    stdx::StringView file(where.file_name(), std::char_traits<char>::length(where.file_name()));
    InitLog.fatalf( fn, " (", file, ":", where.line(), ")\n");
    DumpCurrentStack();
    KmHalt();
}

#pragma clang diagnostic pop
