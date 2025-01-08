#pragma once

#include "std/span.hpp"
#include "std/string_view.hpp"
#include "std/static_string.hpp"
#include "std/traits.hpp"

#include <span>
#include <utility>

namespace km {
    template<typename T>
    struct Format;

    class IOutStream {
    public:
        virtual ~IOutStream() = default;

        void operator delete(IOutStream*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual void write(stdx::StringView message) = 0;
        virtual void write(char c) {
            char buffer[1] = { c };
            write(buffer);
        }
    };

    template<typename T>
    concept IsFormatWithSize = requires {
        { Format<T>::kStringSize } -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept IsFormat = IsFormatWithSize<T> && requires(T it) {
        { Format<T>::toString(std::declval<char*>(), it) } -> std::same_as<stdx::StringView>;
    };

    /// @brief Does a type conform to the static format concept.
    /// @warning While this trait expects toString to return something that is convertible to a string view.
    ///          The returned object is allowed to own its storage, always assign it to auto in cases where
    ///          its a StaticString or similar.
    template<typename T>
    concept IsFormatEx = requires(T it) {
        { Format<T>::toString(it) } -> std::convertible_to<stdx::StringView>;
    };

    template<typename T>
    concept IsStreamFormat = requires(T it) {
        { Format<T>::format(std::declval<IOutStream&>(), it) };
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
        bool prefix = true;

        Hex(T value) : value(value) {}

        Hex pad(size_t width, char fill, bool prefix = true) const {
            Hex copy = *this;
            copy.width = width;
            copy.fill = fill;
            copy.prefix = prefix;
            return copy;
        }
    };

    struct HexDump {
        std::span<const std::byte> data;

        HexDump(std::span<const std::byte> data) : data(data) {}

        template<typename T>
        HexDump(std::span<const T> data)
            : data(reinterpret_cast<const std::byte*>(data.data()), data.size_bytes())
        { }
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

    template<std::integral T>
    stdx::StaticString<stdx::NumericTraits<T>::kMaxDigits16> FormatInt(T value, int base = 10, int width = 0, char fill = '\0') {
        stdx::StaticString<stdx::NumericTraits<T>::kMaxDigits16> result;

        char buffer[stdx::NumericTraits<T>::kMaxDigits16];
        result = FormatInt(stdx::Span(buffer, buffer + stdx::NumericTraits<T>::kMaxDigits16), value, base, width, fill);

        return result;
    }

    constexpr stdx::StringView present(bool present) {
        using namespace stdx::literals;
        return present ? "Present"_sv : "Not Present"_sv;
    }

    constexpr stdx::StringView enabled(bool enabled) {
        using namespace stdx::literals;
        return enabled ? "Enabled"_sv : "Disabled"_sv;
    }

    template<>
    struct Format<char> {
        static constexpr size_t kStringSize = 1;
        static constexpr stdx::StringView toString(char *buffer, char value) {
            buffer[0] = value;
            return stdx::StringView(buffer, buffer + 1);
        }
    };

    template<std::integral T>
    struct Format<T> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static constexpr stdx::StringView toString(char *buffer, T value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value, 10);
        }
    };

    template<std::integral T>
    struct Format<Hex<T>> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits16 + 2;
        static stdx::StringView toString(char *buffer, Hex<T> value) {
            char temp[stdx::NumericTraits<T>::kMaxDigits16];
            stdx::StringView result = FormatInt(stdx::Span(temp), value.value, 16, value.width, value.fill);

            int offset = 0;
            if (value.prefix) {
                buffer[offset++] = '0';
                buffer[offset++] = 'x';
            }

            std::copy(result.begin(), result.end(), buffer + offset);
            return stdx::StringView(buffer, buffer + offset + result.count());
        }
    };

    template<>
    struct Format<void*> {
        static constexpr size_t kStringSize = stdx::NumericTraits<uintptr_t>::kMaxDigits16 + 2;
        static stdx::StringView toString(char *buffer, void *value) {
            return Format<Hex<uintptr_t>>::toString(buffer, Hex<uintptr_t> { reinterpret_cast<uintptr_t>(value) });
        }
    };

    template<std::integral T>
    struct Format<Int<T>> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static stdx::StringView toString(char *buffer, Int<T> value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value.value, 10, value.width, value.fill);
        }
    };

    template<>
    struct Format<bool> {
        static stdx::StringView toString(bool value) {
            using namespace stdx::literals;
            return value ? "True"_sv : "False"_sv;
        }
    };

    template<>
    struct Format<HexDump> {
        static void format(IOutStream& out, HexDump value);
    };

    template<IsFormat T>
    inline constexpr size_t kFormatSize = Format<T>::kStringSize;

    template<IsFormat T>
    inline constexpr stdx::StringView format(char *buffer, T value) {
        return Format<T>::toString(buffer, value);
    }

    template<IsFormat T>
    inline constexpr stdx::StaticString<kFormatSize<T>> format(T value) {
        stdx::StaticString<kFormatSize<T>> result;

        char buffer[kFormatSize<T>];
        result = format(buffer, value);

        return result;
    }

    template<IsFormatEx T>
    inline constexpr auto format(T value) {
        return Format<T>::toString(value);
    }

    enum class Align {
        eLeft,
        eRight,
    };

    struct FormatSpecifier {
        Align align = Align::eRight;
        unsigned width = 0;
        char fill = ' ';
    };

    template<IsFormat T>
    struct FormatOf {
        T value;
        FormatSpecifier specifier;
    };

    struct FormatBuilder {
        FormatSpecifier values;

        template<IsFormat T>
        FormatOf<T> operator+(T value) const {
            return { value, values };
        }
    };

    constexpr FormatBuilder lpad(unsigned width, char fill = ' ') {
        return { { Align::eLeft, width, fill } };
    }

    constexpr FormatBuilder rpad(unsigned width, char fill = ' ') {
        return { { Align::eRight, width, fill } };
    }
}
