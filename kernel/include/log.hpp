#pragma once

#include "kernel.hpp"

inline void KmDebugWrite(stdx::StringView value = "\n") {
    GetDebugStream()->write(value);
}

template<km::IsFormat T>
void KmDebugWrite(const T& value) {
    char buffer[km::Format<T>::kStringSize];
    KmDebugWrite(km::Format<T>::toString(buffer, value));
}

template<km::IsFormatEx T>
void KmDebugWrite(const T& value) {
    auto result = km::format(value);
    KmDebugWrite(result);
}

template<km::IsStreamFormat T>
void KmDebugWrite(const T& value) {
    km::Format<T>::format(*GetDebugStream(), value);
}

template<km::IsFormat T>
void KmDebugWrite(km::FormatOf<T> value) {
    char buffer[km::Format<T>::kStringSize];
    stdx::StringView result = km::Format<T>::toString(buffer, value.value);
    if (value.specifier.width <= result.count()) {
        KmDebugWrite(result);
        return;
    }

    if (value.specifier.align == km::Align::eRight) {
        KmDebugWrite(result);
        for (unsigned i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
    } else {
        for (unsigned i = result.count(); i < value.specifier.width; i++) {
            KmDebugWrite(value.specifier.fill);
        }
        KmDebugWrite(result);
    }
}

template<typename... T>
void KmDebugMessage(T&&... args) {
    KmBeginWrite();
    (KmDebugWrite(args), ...);
    KmEndWrite();
}
