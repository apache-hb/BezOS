#pragma once

#include "arch/intrin.hpp"
#include "util/format.hpp"
#include "util/util.hpp"
#include <stdint.h>

namespace x64 {
    using cr0_t = uint32_t;

    class Cr0 {
        cr0_t mValue;

        constexpr Cr0(cr0_t value) : mValue(value) { }

    public:
        constexpr Cr0() : Cr0(0) { }

        enum Bit : cr0_t {
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

        constexpr cr0_t value() const { return mValue; }

        constexpr static Cr0 of(cr0_t value) {
            return Cr0(value);
        }

        constexpr bool test(Bit flag) const {
            return mValue & flag;
        }

        constexpr void set(Bit flag) {
            mValue |= flag;
        }

        constexpr void clear(Bit flag) {
            mValue &= ~flag;
        }

        [[gnu::always_inline, gnu::nodebug, nodiscard]]
        static Cr0 load() {
            return Cr0(__get_cr0());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(Cr0 cr0) [[clang::blocking]] {
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
