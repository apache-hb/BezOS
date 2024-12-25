#pragma once

#include <stddef.h>

#include "std/string_view.hpp"
#include "std/traits.hpp"

#include "util/format.hpp"

void KmDebugWrite(stdx::StringView value = "\n");

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

template<km::IsStaticFormat T>
void KmDebugWrite(km::FormatOf<T> value) {
    char buffer[km::StaticFormat<T>::kStringSize];
    stdx::StringView result = km::StaticFormat<T>::toString(buffer, value.value);
    if (value.specifier.width <= result.count()) {
        KmDebugWrite(result);
        return;
    }

    if (value.specifier.align == km::Align::eRight) {
        KmDebugWrite(result);
        for (int i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
    } else {
        for (int i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
        KmDebugWrite(result);
    }
}

template<typename... T>
void KmDebugMessage(T&&... args) {
    (KmDebugWrite(args), ...);
}

void KmSetupGdt();

extern "C" [[noreturn]] void KmHalt(void);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
