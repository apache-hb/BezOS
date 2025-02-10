#pragma once

#include "arch/intrin.hpp"
#include "util/util.hpp"

#include <stdint.h>

namespace x64 {
    enum class RegisterAccess {
        eRead = (1 << 0),
        eWrite = (1 << 1),

        eReadWrite = eRead | eWrite,
    };

    UTIL_BITFLAGS(RegisterAccess);

    template<uint32_t R, RegisterAccess A>
    struct ModelRegister {
        static constexpr uint32_t kRegister = R;
        static constexpr RegisterAccess kAccess = A;

        /// @brief Load the value of the register.
        /// @return The value of the register.
        [[gnu::always_inline, gnu::nodebug]]
        uint64_t load() const requires (bool(A & RegisterAccess::eRead)) {
            return __rdmsr(kRegister);
        }

        /// @brief Store a new value in the register.
        /// @param value The new value to store.
        [[gnu::always_inline, gnu::nodebug]]
        void store(uint64_t value) const requires (bool(A & RegisterAccess::eWrite)) {
            __wrmsr(kRegister, value);
        }

        /// @brief Update the value of the register.
        /// Only writes the new value if it is different from the old value.
        /// @param old The old value of the register.
        /// @param modify The function to modify the value.
        void update(uint64_t& old, auto modify) const requires (bool(A & RegisterAccess::eWrite)) {
            uint64_t value = old;
            modify(value);

            if (value != old) {
                store(value);
                old = value;
            }
        }

        /// @brief Set a bitmask of bits in the register.
        /// Only writes the new value if it is different from the old value.
        /// @param old The old value of the register.
        /// @param mask The bitmask to set.
        /// @param set Whether to set or clear the bits.
        void updateBits(uint64_t& old, uint64_t mask, bool set) const requires (bool(A & RegisterAccess::eWrite)) {
            update(old, [&](uint64_t& value) {
                if (set) {
                    value |= mask;
                } else {
                    value &= ~mask;
                }
            });
        }

        void operator|=(uint64_t mask) const requires (bool(A & RegisterAccess::eWrite)) {
            store(load() | mask);
        }
        
        void operator&=(uint64_t mask) const requires (bool(A & RegisterAccess::eWrite)) {
            store(load() & mask);
        }
    };

    template<uint64_t B>
    struct RegisterBit {

    };
}
