#pragma once

#include <bit>
#include <concepts>

#include "util/digit.hpp"

namespace km {
    template<size_t N, typename T>
    constexpr SizedInteger<N, T> byteswap(SizedInteger<N, T> value) {
        return value.byteswap();
    }

    template<std::integral T>
    constexpr T byteswap(T value) {
        return std::byteswap(value);
    }

    /// @brief Endian type
    ///
    /// Endian aware value for storing types with a consistent byte order.
    ///
    /// @tparam T the type of the value
    /// @tparam Order the desired byte order of the value
    template<typename T, std::endian Order>
    struct EndianValue {
        constexpr EndianValue() = default;

        /// @brief Construct a new Endian Value object
        ///
        /// Converts the value to the desired byte order before storing
        ///
        /// @param v the value to store
        constexpr EndianValue(T v) {
            store(v);
        }

        /// @brief Load the value in native byte order
        ///
        /// @return T the value in native byte order
        constexpr operator T() const {
            return load();
        }

        /// @brief Load the value in the desired byte order
        ///
        /// @return T the value in the desired byte order
        constexpr T load() const {
            if constexpr (Order == std::endian::native) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        /// @brief Store a new value in the desired byte order
        ///
        /// @param v the value to store
        constexpr void store(T v) {
            if constexpr (Order == std::endian::native) {
                underlying = v;
            } else {
                underlying = byteswap(v);
            }
        }

        /// @brief Assign a new value of native byte order
        ///
        /// Converts the value to the desired byte order before storing
        ///
        /// @param v the value to assign
        constexpr EndianValue& operator=(T v) {
            store(v);
            return *this;
        }

        /// @brief Load the value in little endian byte order
        ///
        /// @return T the value in little endian byte order
        constexpr T little() const {
            if constexpr (Order == std::endian::little) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        /// @brief Load the value in big endian byte order
        ///
        /// @return T the value in big endian byte order
        constexpr T big() const {
            if constexpr (Order == std::endian::big) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        /// @brief The underlying representation
        T underlying;
    };

    /// @brief A big endian value
    ///
    /// @tparam T the type of the value
    template<typename T>
    using be = EndianValue<T, std::endian::big>;

    /// @brief A little endian value
    ///
    /// @tparam T the type of the value
    template<typename T>
    using le = EndianValue<T, std::endian::little>;
}
