#pragma once

#include <bezos/status.h>

#include "std/span.hpp"
#include "std/string_view.hpp"
#include "std/static_string.hpp"
#include "std/traits.hpp"

#include "common/compiler/compiler.hpp"

#include <span>
#include <utility>

namespace km {
    template<typename T>
    struct Format;

    template<typename T>
    concept IsFormatSize = requires {
        { Format<T>::kStringSize } -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept IsFormat = IsFormatSize<T> && requires(T it) {
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
        { Format<T>::format(std::declval<class IOutStream&>(), it) };
    };

    class IOutStream {
    public:
        virtual ~IOutStream() = default;

        virtual void write(stdx::StringView message) [[clang::nonreentrant]] = 0;
        virtual void write(char c) {
            write(std::to_array({ c }));
        }

        template<typename T> requires (!std::convertible_to<T, stdx::StringView>)
        void write(const T& value);

        template<typename... T>
        void format(T&&... args) {
            (write(std::forward<T>(args)), ...);
        }
    };

    template<std::integral T>
    struct Int {
        T value;
        int width = 0;
        char fill = '\0';

        Int(T value) noexcept [[clang::nonblocking]] : value(value) {}

        Int pad(size_t width, char fill = '0') const {
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

        Hex(T value) noexcept [[clang::reentrant, clang::nonblocking]] : value(value) {}

        Hex pad(size_t width, char fill = '0', bool prefix = true) const {
            Hex copy = *this;
            copy.width = width;
            copy.fill = fill;
            copy.prefix = prefix;
            return copy;
        }
    };

    struct HexDump {
        std::span<const std::byte> data;
        uintptr_t base;

        HexDump(std::span<const std::byte> data, uintptr_t base)
            : data(data)
            , base(base)
        { }

        HexDump(std::span<const std::byte> data)
            : HexDump(data, (uintptr_t)data.data())
        { }

        template<typename T>
        HexDump(std::span<const T> data)
            : HexDump(std::span(reinterpret_cast<const std::byte*>(data.data()), data.size_bytes()))
        { }

        template<typename T>
        HexDump(std::span<const T> data, uintptr_t base)
            : HexDump(std::span(reinterpret_cast<const std::byte*>(data.data()), data.size_bytes()), base)
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
                value = (stdx::Unsigned<T>)input;
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

        static void format(IOutStream& out, char value) {
            out.write(value);
        }
    };

    template<std::integral T>
    struct Format<T> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static constexpr stdx::StringView toString(char *buffer, T value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value, 10);
        }

        static void format(IOutStream& out, T value) {
            char buffer[kStringSize];
            out.write(toString(buffer, value));
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

        static void format(IOutStream& out, Hex<T> value) {
            char buffer[kStringSize];
            out.write(toString(buffer, value));
        }
    };

    template<>
    struct Format<const void*> {
        static constexpr size_t kStringSize = stdx::NumericTraits<uintptr_t>::kMaxDigits16 + 2;
        static stdx::StringView toString(char *buffer, const void *value) {
            return Format<Hex<uintptr_t>>::toString(buffer, Hex<uintptr_t> { reinterpret_cast<uintptr_t>(value) });
        }

        static void format(IOutStream& out, void *value) {
            char buffer[kStringSize];
            out.write(toString(buffer, value));
        }
    };

    template<>
    struct Format<void*> {
        static constexpr size_t kStringSize = stdx::NumericTraits<uintptr_t>::kMaxDigits16 + 2;
        static stdx::StringView toString(char *buffer, const void *value) {
            return Format<Hex<uintptr_t>>::toString(buffer, Hex<uintptr_t> { reinterpret_cast<uintptr_t>(value) });
        }

        static void format(IOutStream& out, const void *value) {
            char buffer[kStringSize];
            out.write(toString(buffer, value));
        }
    };

    template<std::integral T>
    struct Format<Int<T>> {
        static constexpr size_t kStringSize = stdx::NumericTraits<T>::kMaxDigits10;
        static stdx::StringView toString(char *buffer, Int<T> value) {
            return FormatInt(stdx::Span(buffer, buffer + kStringSize), value.value, 10, value.width, value.fill);
        }

        static void format(IOutStream& out, Int<T> value) {
            char buffer[kStringSize];
            out.write(toString(buffer, value));
        }
    };

    template<>
    struct Format<bool> {
        static stdx::StringView toString(bool value) {
            using namespace stdx::literals;
            return value ? "True"_sv : "False"_sv;
        }

        static void format(IOutStream& out, bool value) {
            out.write(toString(value));
        }
    };

    template<>
    struct Format<HexDump> {
        static void format(IOutStream& out, HexDump value);
    };

    template<IsFormatSize T>
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

    template<IsStreamFormat T>
    inline void format(IOutStream& out, const T& value) noexcept {
        Format<T>::format(out, value);
    }

    inline void format(IOutStream& out, stdx::StringView value) noexcept {
        out.write(value);
    }

    template<size_t N, IsStreamFormat T>
    inline stdx::StaticString<N> toStaticString(const T& value) {
        struct OutStream final : public IOutStream {
            stdx::StaticString<N> result;

            void write(stdx::StringView message) noexcept [[clang::reentrant]] override {
                result.add(message);
            }
        };

        OutStream out;
        out.format(value);

        return out.result;
    }

    template<size_t N, typename... T>
    inline stdx::StaticString<N> concat(T&&... args) noexcept [[clang::reentrant, clang::nonallocating]] {
        struct OutStream final : public IOutStream {
            stdx::StaticString<N> result;

            void write(stdx::StringView message) noexcept [[clang::reentrant, clang::nonallocating]] override {
                result.add(message);
            }
        };

        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
        // Formatting may not be reentrant, but only if the format function is not reentrant.
        // In this case we control the format function and know it is reentrant.

        OutStream out;
        (out.format(args), ...);

        CLANG_DIAGNOSTIC_POP();

        return out.result;
    }

    template<typename T> requires (IsStreamFormat<T> && IsFormatSize<T> && !IsFormat<T>)
    inline constexpr auto format(T value) {
        return toStaticString<kFormatSize<T>>(value);
    }

    template<IsFormatEx T> requires (!IsStreamFormat<T>)
    inline void format(IOutStream& out, const T& value) {
        out.write(Format<T>::toString(value));
    }

    template<IsFormat T> requires (!IsStreamFormat<T>)
    inline void format(IOutStream& out, const T& value) {
        char buffer[kFormatSize<T>];
        out.write(Format<T>::toString(buffer, value));
    }

    template<typename T> requires (!std::convertible_to<T, stdx::StringView>)
    void IOutStream::write(const T& value) {
        km::format(*this, value);
    }

    template<>
    struct Format<OsStatusId> {
        // 16 for the extra message attatched to the hex code
        // TODO: this should be done with an X-macro so the size can be known here
        static constexpr size_t kStringSize = kFormatSize<km::Hex<OsStatus>> + 16;

        static void format(IOutStream& out, OsStatusId value);
    };
}
