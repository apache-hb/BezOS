#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>

namespace sm {
    // bitset handling functions modified from reiser4

    struct BitCount {
        size_t count;
    };

    template<typename T>
    BitCount BitSizeOf() {
        return BitCount { sizeof(T) * CHAR_MAX };
    }

    void BitsSetRange(uint8_t *words, BitCount front, BitCount back);
    void BitsClearRange(uint8_t *words, BitCount front, BitCount back);
    BitCount BitsFindNextSet(uint8_t *words, BitCount start, BitCount size);
    BitCount BitsFindAndSetNextFree(uint8_t *words, BitCount start, BitCount size);
    BitCount BitsFindFreeRange(uint8_t *words, BitCount start, BitCount length, BitCount size);
    BitCount BitsFindAndSetFreeRange(uint8_t *words, BitCount start, BitCount length, BitCount size);
    void BitsSetBit(uint8_t *words, BitCount bit);
    void BitsClearBit(uint8_t *words, BitCount bit);

    /// @brief Create a bitmask for a range of bits.
    ///
    /// Create a bitmask where all bits in the range [@p first, @p last) are set.
    ///
    /// @tparam T The type of the bitmask.
    /// @param first The first bit of the range.
    /// @param last The last bit of the range.
    /// @return The bitmask.
    template<typename T>
    T BitMask(size_t first, size_t last) {
        size_t length = last - first;
        T mask = (1 << length) - 1;
        return mask << first;
    }

    /// @brief Extract a range of bits from a value.
    ///
    /// Extract [@p front, @p back) from @p value.
    ///
    /// @tparam T The type of the value.
    /// @param value The value to extract from.
    /// @param front The first bit of the range.
    /// @param back The last bit of the range.
    /// @return The extracted value.
    template<typename T>
    T BitExtract(T value, size_t front, size_t back) {
        T mask = BitMask<T>(front, back);
        return (value & mask) >> front;
    }
}
