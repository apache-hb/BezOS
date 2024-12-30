#pragma once

#include "std/span.hpp"
#include "std/string_view.hpp"
#include "std/static_string.hpp"
#include "std/traits.hpp"

namespace km {
    template<typename T>
    struct StaticFormat;

    template<typename T>
    concept IsStaticFormat = requires(T it) {
        { StaticFormat<T>::kStringSize } -> std::convertible_to<size_t>;
        { StaticFormat<T>::toString(std::declval<char*>(), it) } -> std::same_as<stdx::StringView>;
    };

    template<typename T>
    concept IsStaticFormatEx = requires(T it) {
        { StaticFormat<T>::toString(it) } -> stdx::StaticStringType;
    };

    template<std::integral T>
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

    template<std::integral T>
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

    template<std::integral T>
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

    constexpr stdx::StringView present(bool present) {
        return present ? stdx::StringView("Present") : stdx::StringView("Not present");
    }

    constexpr stdx::StringView enabled(bool enabled) {
        return enabled ? stdx::StringView("Enabled") : stdx::StringView("Disabled");
    }

    template<>
    struct StaticFormat<char> {
        static constexpr size_t kStringSize = 1;
        static constexpr stdx::StringView toString(char *buffer, char value) {
            buffer[0] = value;
            return stdx::StringView(buffer, buffer + 1);
        }
    };

    template<std::integral T>
    struct StaticFormat<T> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static constexpr stdx::StringView toString(char *buffer, T value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value, 10);
        }
    };

    template<std::integral T>
    struct StaticFormat<Hex<T>> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits16 + 2;
        static stdx::StringView toString(char *buffer, Hex<T> value) {
            char temp[stdx::NumericTraits<T>::kMaxDigits16];
            stdx::StringView result = FormatInt(stdx::Span(temp), value.value, 16, value.width, value.fill);
            buffer[0] = '0';
            buffer[1] = 'x';
            std::copy(result.begin(), result.end(), buffer + 2);
            return stdx::StringView(buffer, buffer + 2 + result.count());
        }
    };

    template<std::integral T>
    struct StaticFormat<Int<T>> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static stdx::StringView toString(char *buffer, Int<T> value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value.value, 10, value.width, value.fill);
        }
    };

    template<>
    struct StaticFormat<bool> {
        static constexpr size_t kStringSize = 8;
        static stdx::StringView toString(char*, bool value) {
            return value ? stdx::StringView("true") : stdx::StringView("false");
        }
    };

    template<IsStaticFormat T>
    inline constexpr size_t kFormatSize = StaticFormat<T>::kStringSize;

    template<IsStaticFormat T>
    inline constexpr stdx::StringView format(char *buffer, T value) {
        return StaticFormat<T>::toString(buffer, value);
    }

    template<IsStaticFormat T>
    inline constexpr stdx::StaticString<kFormatSize<T>> format(T value) {
        stdx::StaticString<kFormatSize<T>> result;

        char buffer[kFormatSize<T>];
        result = format(buffer, value);

        return result;
    }

    template<IsStaticFormatEx T>
    inline constexpr auto format(T value) {
        return StaticFormat<T>::toString(value);
    }

    enum class Align {
        eLeft,
        eRight,
    };

    struct FormatSpecifier {
        Align align = Align::eRight;
        int width = 0;
        char fill = ' ';
    };

    template<IsStaticFormat T>
    struct FormatOf {
        T value;
        FormatSpecifier specifier;
    };

    struct FormatBuilder {
        FormatSpecifier values;

        template<IsStaticFormat T>
        FormatOf<T> operator+(T value) const {
            return { value, values };
        }
    };

    constexpr FormatBuilder lpad(int width, char fill = ' ') {
        return { { Align::eLeft, width, fill } };
    }

    constexpr FormatBuilder rpad(int width, char fill = ' ') {
        return { { Align::eRight, width, fill } };
    }
}
