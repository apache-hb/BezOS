#pragma once

#include <stddef.h>

#include "arch/intrin.hpp"
#include "std/string_view.hpp"

#include "util/format.hpp"

void KmDebugWrite(stdx::StringView value = "\n");

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

inline void KmDelayIo() {
    __outbyte(0x80, 0);
}

inline void __DEFAULT_FN_ATTRS KmWriteByte(uint16_t port, uint8_t value) {
    __outbyte(port, value);
    KmDelayIo();
}

inline void __DEFAULT_FN_ATTRS KmWriteByteNoDelay(uint16_t port, uint8_t value) {
    __outbyte(port, value);
}

inline uint8_t __DEFAULT_FN_ATTRS KmReadByte(uint16_t port) {
    return __inbyte(port);
}

extern "C" [[noreturn]] void KmHalt(void);

[[noreturn]]
void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line);

#define KM_PANIC(msg) KmBugCheck(msg, __FILE__, __LINE__)
#define KM_CHECK(expr, msg) do { if (!(expr)) { KmBugCheck(msg, __FILE__, __LINE__); } } while (0)
