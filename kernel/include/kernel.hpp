#pragma once

#include <stddef.h>

#include "std/string_view.hpp"
#include "std/traits.hpp"

#include "util/format.hpp"

void KmDebugWrite(stdx::StringView value = "\n");

template<stdx::Integer T>
void KmDebugWrite(T value) {
    char buffer[stdx::NumericTraits<T>::kMaxDigits10];
    KmDebugWrite(km::FormatInt(buffer, value, 10));
}

template<stdx::Integer T>
void KmDebugWrite(km::Int<T> value) {
    char buffer[stdx::NumericTraits<T>::kMaxDigits10];
    KmDebugWrite(km::FormatInt(buffer, value.value, 10, value.width, value.fill));
}

template<stdx::Integer T>
void KmDebugWrite(km::Hex<T> value) {
    char buffer[stdx::NumericTraits<T>::kMaxDigits16];
    KmDebugWrite("0x");
    KmDebugWrite(km::FormatInt(buffer, value.value, 16, value.width, value.fill));
}

template<km::IsStaticFormat T>
void KmDebugWrite(const T& value) {
    char buffer[km::StaticFormat<T>::kStringSize];
    KmDebugWrite(km::StaticFormat<T>::toString(buffer, value));
}

template<km::IsFormat T>
void KmDebugWrite(const T& value) {
    char buffer[T::kStringSize];
    KmDebugWrite(value.toString(buffer));
}

template<typename... T>
void KmDebugMessage(T&&... args) {
    (KmDebugWrite(args), ...);
}

void KmInitGdt();

extern "C" [[noreturn]] void KmHalt(void);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
