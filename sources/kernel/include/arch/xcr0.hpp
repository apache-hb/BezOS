#pragma once

#include "arch/intrin.hpp"
#include "util/util.hpp"
#include "util/format.hpp"

#include <stdint.h>

namespace x64 {
    using xcr0_t = uint64_t;

    class Xcr0 {
        xcr0_t mValue;

        constexpr Xcr0(xcr0_t value) : mValue(value) { }

    public:
        enum Bit : xcr0_t {
            FPU = (1 << 0),
            SSE = (1 << 1),
            AVX = (1 << 2),
            MPX = (0b11 << 3),
            AVX512 = (0b111 << 5),
            PKRU = (1 << 9),
            AMX = (0b11 << 17),
        };

        constexpr xcr0_t value() const { return mValue; }

        constexpr static Xcr0 of(xcr0_t value) {
            return Xcr0(value);
        }

        constexpr bool test(Bit flag) const {
            return mValue & flag;
        }

        constexpr void set(Bit flag) {
            mValue |= flag;
        }

        constexpr void clear(Bit bit) {
            mValue &= ~bit;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static Xcr0 load() {
            return Xcr0(__get_xcr0());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(const Xcr0& xcr0) {
            __set_xcr0(xcr0.value());
        }
    };

    UTIL_BITFLAGS(Xcr0::Bit);
}

template<>
struct km::Format<x64::Xcr0> {
    static void format(km::IOutStream& out, x64::Xcr0 value);
};
