#pragma once

#include <cstdint>
#include <cstring>

#include <limits>

#include <algorithm>

namespace sm {
    namespace detail {
        template<size_t N, typename T>
        struct SizedIntegerTraits { using type = void; };

        template<> struct SizedIntegerTraits<1, signed> { using type = int8_t; };
        template<> struct SizedIntegerTraits<2, signed> { using type = int16_t; };

        template<> struct SizedIntegerTraits<3, signed> { using type = int32_t; };
        template<> struct SizedIntegerTraits<4, signed> { using type = int32_t; };

        template<> struct SizedIntegerTraits<5, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<6, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<7, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<8, signed> { using type = int64_t; };

        template<> struct SizedIntegerTraits<1, unsigned> { using type = uint8_t; };
        template<> struct SizedIntegerTraits<2, unsigned> { using type = uint16_t; };

        template<> struct SizedIntegerTraits<3, unsigned> { using type = uint32_t; };
        template<> struct SizedIntegerTraits<4, unsigned> { using type = uint32_t; };

        template<> struct SizedIntegerTraits<5, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<6, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<7, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<8, unsigned> { using type = uint64_t; };
    }

    /// @brief Non power of 2 sized integer
    ///
    /// A fixed size integer type that is not a power of 2 in byte size.
    ///
    /// @tparam N the number of bytes in the integer
    /// @tparam S the signedness of the integer
    template<size_t N, typename S> requires (N > 0 && N <= 8)
    struct [[gnu::packed]] SizedInteger {
        /// @brief The octets that make up the integer
        uint8_t octets[N];

        /// @brief The smallest integral type that can hold all values of the SizedInteger
        using Integral = typename detail::SizedIntegerTraits<N, S>::type;

        /// @brief Is the integer signed
        static constexpr bool kIsSigned = std::is_signed_v<S>;

        /// @brief The maximum value that can be stored in the integer
        static constexpr Integral kMax = kIsSigned ? (1LL << (N * 8 - 1)) - 1 : (1ull << (N * 8)) - 1;

        /// @brief The minimum value that can be stored in the integer
        static constexpr Integral kMin = kIsSigned ? -(1LL << (N * 8 - 1)) : 0;

        constexpr SizedInteger() = default;

        /// @brief Construct a new Sized Integer object
        ///
        /// @param value the value to store
        /// @note @a value is clamped to @ref kMin and @ref kMax
        constexpr SizedInteger(Integral value) {
            store(value);
        }

        /// @brief Load the value of the integer
        ///
        /// @return Integral the value of the integer
        constexpr operator Integral() const {
            return load();
        }

        /// @brief Load the value of the integer
        ///
        /// @return Integral the value of the integer
        constexpr Integral load() const {
            Integral value = 0;
            for (size_t i = 0; i < N; i++) {
                value |= octets[i] << (i * 8);
            }
            return value;
        }

        /// @brief Store a new value in the integer
        ///
        /// @param value the value to store
        /// @note @a value is clamped to @ref kMin and @ref kMax
        constexpr void store(Integral value) {
            value = std::clamp(value, kMin, kMax);
            for (size_t i = 0; i < N; i++) {
                octets[i] = (value >> (i * 8)) & 0xFF;
            }
        }

        /// @brief Assign a new value to the integer
        ///
        /// @param value the value to store
        /// @note @a value is clamped to @ref kMin and @ref kMax
        constexpr SizedInteger& operator=(Integral value) {
            store(value);
            return *this;
        }

        /// @brief Byteswap the integer
        ///
        /// @return SizedInteger the byteswapped integer
        constexpr SizedInteger byteswap() const {
            SizedInteger result;
            std::reverse_copy(octets, octets + N, result.octets);
            return result;
        }
    };

    using uint24_t = SizedInteger<3, unsigned>;
    using int24_t = SizedInteger<3, signed>;

    using uint40_t = SizedInteger<5, unsigned>;
    using int40_t = SizedInteger<5, signed>;

    using uint48_t = SizedInteger<6, unsigned>;
    using int48_t = SizedInteger<6, signed>;

    using uint56_t = SizedInteger<7, unsigned>;
    using int56_t = SizedInteger<7, signed>;

    static_assert(sizeof(uint24_t) == 3);
    static_assert(sizeof(int24_t) == 3);

    static_assert(sizeof(uint40_t) == 5);
    static_assert(sizeof(int40_t) == 5);

    static_assert(sizeof(uint48_t) == 6);
    static_assert(sizeof(int48_t) == 6);

    static_assert(sizeof(uint56_t) == 7);
    static_assert(sizeof(int56_t) == 7);
}

template<size_t N, typename S> requires (N > 0 && N <= 8)
struct std::numeric_limits<sm::SizedInteger<N, S>> {
    using Type = sm::SizedInteger<N, S>;
    static constexpr Type min() noexcept { return Type::kMin; }
    static constexpr Type max() noexcept { return Type::kMax; }

    static constexpr bool is_integer = true;
    static constexpr int digits10 = N * 8 - 1;
    static constexpr bool is_exact = true;
    static constexpr bool is_signed = Type::kIsSigned;
};
