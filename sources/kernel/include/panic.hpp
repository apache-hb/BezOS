#pragma once

#include "std/string_view.hpp"

#include <source_location>

extern "C" [[noreturn]] void KmHalt(void);
extern "C" [[noreturn]] void KmIdle(void);

namespace km {
    [[noreturn]]
    void BugCheck(stdx::StringView message, std::source_location where = std::source_location::current()) noexcept [[clang::nonblocking]];
}

#define KM_PANIC(msg) km::BugCheck(msg)
#define KM_CHECK(expr, msg) do { if (!(expr)) { km::BugCheck(msg); } } while (0)
#define KM_ASSERT(expr) do { if (!(expr)) { km::BugCheck(#expr); } } while (0)
