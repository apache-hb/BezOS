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
    BitCount BitsFindFreeRange(uint8_t *words, BitCount start, BitCount length, BitCount size);
    BitCount BitsFindAndSetFreeRange(uint8_t *words, BitCount start, BitCount length, BitCount size);
}
