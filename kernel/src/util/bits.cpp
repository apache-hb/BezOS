#include "util/bits.hpp"

void sm::BitsSetRange(uint8_t *words, BitCount front, BitCount back) {
    for (size_t i = front.count; i < back.count; i++) {
        words[i / CHAR_BIT] |= 1 << (i % CHAR_BIT);
    }
}

void sm::BitsClearRange(uint8_t *words, BitCount front, BitCount back) {
    for (size_t i = front.count; i < back.count; i++) {
        words[i / CHAR_BIT] &= ~(1 << (i % CHAR_BIT));
    }
}

sm::BitCount sm::BitsFindNextSet(uint8_t *words, BitCount start, BitCount size) {
    for (size_t i = start.count; i < size.count; i++) {
        if (words[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
            return BitCount { i };
        }
    }

    return size;
}

sm::BitCount sm::BitsFindFreeRange(uint8_t *words, BitCount start, BitCount size, BitCount count) {
    size_t front = start.count;
    size_t length = 0;

    for (size_t i = start.count; i < size.count; i++) {
        if (words[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
            front = i + 1;
            length = 0;
        } else {
            length += 1;
            if (length == count.count) {
                return BitCount { front };
            }
        }
    }

    return size;
}

sm::BitCount sm::BitsFindAndSetFreeRange(uint8_t *words, BitCount start, BitCount length, BitCount size) {
    BitCount front = BitsFindFreeRange(words, start, size, length);
    if (front.count < size.count) {
        BitsSetRange(words, front, BitCount { front.count + length.count });
    }

    return front;
}

void sm::BitsSetBit(uint8_t *words, BitCount bit) {
    words[bit.count / CHAR_BIT] |= 1 << (bit.count % CHAR_BIT);
}

void sm::BitsClearBit(uint8_t *words, BitCount bit) {
    words[bit.count / CHAR_BIT] &= ~(1 << (bit.count % CHAR_BIT));
}
