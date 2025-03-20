#pragma once

#include "arch/intrin.hpp"
#include "util/format.hpp"
#include "util/util.hpp"
#include <stdint.h>

namespace x64 {
    using cr3_t = uintptr_t;

    using pcid_t = uint16_t;

    class Cr3 {
        cr3_t mValue;

        constexpr Cr3(cr3_t value) : mValue(value) { }

    public:
        constexpr Cr3() : Cr3(0) { }

        enum Bit : cr3_t {
            ePcidMask = (0xFFF),

            /// @brief Largest possible address mask
            eAddressMask = (0x0FFF'FFFF'FFFF'F000),

            PWT = (1ull << 3),
            PCD = (1ull << 4),

            LAM_U57 = (1ull << 61),
            LAM_U48 = (1ull << 62),
        };

        constexpr cr3_t value() const { return mValue; }

        constexpr static Cr3 of(cr3_t value) {
            return Cr3(value);
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

        constexpr pcid_t pcid() const {
            return mValue & ePcidMask;
        }

        constexpr void setPcid(pcid_t pcid) {
            mValue = (mValue & ~ePcidMask) | pcid;
        }

        constexpr uintptr_t address() const {
            return mValue & eAddressMask;
        }

        constexpr void setAddress(uintptr_t address) {
            mValue = (mValue & ~eAddressMask) | (address & eAddressMask);
        }

        [[gnu::always_inline, gnu::nodebug, nodiscard]]
        static Cr3 load() {
            return Cr3(__get_cr3());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(Cr3 v) [[clang::blocking]] {
            __set_cr3(v.value());
        }
    };

    UTIL_BITFLAGS(Cr3::Bit);
}

template<>
struct km::Format<x64::Cr3> {
    static void format(km::IOutStream& out, x64::Cr3 value);
};
