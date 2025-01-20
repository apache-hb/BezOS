#pragma once

#include "arch/intrin.hpp"
#include "util/format.hpp"
#include "util/util.hpp"
#include <stdint.h>

namespace x64 {
    class Cr0 {
        uint32_t mValue;

        Cr0(uint32_t value) : mValue(value) { }

    public:
        enum Bit : uint32_t {
            PG = (1ull << 31),
            CD = (1ull << 30),
            NW = (1ull << 29),
            AM = (1ull << 18),
            WP = (1ull << 16),
            NE = (1ull << 5),
            ET = (1ull << 4),
            TS = (1ull << 3),
            EM = (1ull << 2),
            MP = (1ull << 1),
            PE = (1ull << 0),
        };

        uint32_t value() const { return mValue; }

        static Cr0 of(uint32_t value) {
            return Cr0(value);
        }

        bool test(Bit flag) const {
            return mValue & flag;
        }

        void set(Bit flag) {
            mValue |= flag;
        }

        void clear(Bit flag) {
            mValue &= ~flag;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static Cr0 load() {
            return Cr0(__get_cr0());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(Cr0 cr0) {
            __set_cr0(cr0.value());
        }
    };

    UTIL_BITFLAGS(Cr0::Bit);
}

template<>
struct km::Format<x64::Cr0> {
    using String = stdx::StaticString<64>;
    static String toString(x64::Cr0 value);
};
