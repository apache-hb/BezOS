#pragma once

#include "std/string_view.hpp"

extern "C" [[noreturn]] void KmHalt(void);
extern "C" [[noreturn]] void KmIdle(void);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
