#pragma once

#include "arch/intrin.hpp"
#include "arch/xsave.hpp"

#include "util/format.hpp"

#include <stdint.h>

namespace x64 {
    using xcr0_t = uint64_t;

    class Xcr0 {
        xcr0_t mValue;

        constexpr Xcr0(xcr0_t value) : mValue(value) { }

    public:
        constexpr xcr0_t value() const { return mValue; }

        constexpr static Xcr0 of(xcr0_t value) {
            return Xcr0(value);
        }

        constexpr bool test(XSaveFeature flag) const {
            return mValue & flag;
        }

        constexpr void set(XSaveFeature flag) {
            mValue |= flag;
        }

        constexpr void clear(XSaveFeature bit) {
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
}

template<>
struct km::Format<x64::Xcr0> {
    static void format(km::IOutStream& out, x64::Xcr0 value);
};
