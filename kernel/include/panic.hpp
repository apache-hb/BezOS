#pragma once

#include "std/string_view.hpp"

#include <source_location>

extern "C" [[noreturn]] void KmHalt(void);
extern "C" [[noreturn]] void KmIdle(void);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

namespace km {
    [[noreturn]]
    void BugCheck(stdx::StringView message, std::source_location where = std::source_location::current());
}

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
#define KM_ASSERT(expr) do { if (!(expr)) { km::BugCheck(#expr); } } while (0)
