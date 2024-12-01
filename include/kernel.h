#pragma once

#include <stddef.h>

template<typename T, size_t N>
consteval size_t countof(const T (&)[N]) {
    return N;
}

[[noreturn]]
void KmHalt(void);

void KmDebugMessage(const char *message);

[[noreturn]]
void KmBugCheck(const char *message, const char *file, unsigned line);

#define KM_BUGCHECK(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
