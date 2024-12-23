#pragma once

#include "std/span.hpp"
#include "std/string_view.hpp"
#include "std/traits.hpp"

namespace km {
    template<stdx::Integer T>
    struct Int {
        T value;
        int width = 0;
        char fill = '\0';

        Int(T value) : value(value) {}

        Int pad(size_t width, char fill) const {
            Int copy = *this;
            copy.width = width;
            copy.fill = fill;
            return copy;
        }
    };

    template<stdx::Integer T>
    struct Hex {
        T value;
        int width = 0;
        char fill = '\0';

        Hex(T value) : value(value) {}

        Hex pad(size_t width, char fill) const {
            Hex copy = *this;
            copy.width = width;
            copy.fill = fill;
            return copy;
        }
    };

    template<stdx::Integer T>
    stdx::StringView FormatInt(stdx::Span<char> buffer, T input, int base, int width = 0, char fill = '\0') {
        static constexpr char kHex[] = "0123456789ABCDEF";
        bool negative = input < 0;

        char *ptr = buffer.end() - 1;
        if (input != 0) {
            stdx::Unsigned<T> value;

            if (negative) {
                value = -input;
            } else {
                value = input;
            }

            while (value != 0) {
                *ptr-- = kHex[value % base];
                value /= base;
            }
        } else {
            *ptr-- = '0';
        }

        if (fill != '\0') {
            if (negative) {
                width--;
            }

            int reamining = width - (buffer.end() - ptr) + 1;
            while (reamining-- > 0) {
                *ptr-- = fill;
            }
        }

        if (negative) {
            *ptr-- = '-';
        }

        return stdx::StringView(ptr + 1, buffer.end());
    }

    template<typename T>
    concept IsFormat = requires(T it) {
        { it.toString(stdx::declval<char*>()) } -> stdx::Same<stdx::StringView>;
    };
}
