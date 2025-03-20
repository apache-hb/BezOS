#pragma once

#include "arch/msr.hpp"

namespace x64 {
    template<typename T, unsigned Bit, RegisterAccess Access>
    struct MsrBit {
        static_assert(Bit < 64, "Invalid bit index.");

        class Apply {
            T& mMsr;

        public:
            constexpr Apply(T& msr) : mMsr(msr) {}

            void operator=(bool value) requires (bool(Access & RegisterAccess::eWrite)) {
                uint64_t mask = 1 << Bit;

                if (value) {
                    mMsr |= mask;
                } else {
                    mMsr &= ~mask;
                }
            }

            operator bool() const requires (bool(Access & RegisterAccess::eRead)) {
                return mMsr.load() & (1 << Bit);
            }
        };

        constexpr Apply operator()(T& reg) const {
            return Apply{reg};
        }

        constexpr Apply apply(T& reg) const {
            return Apply{reg};
        }
    };

    template<typename T, unsigned First, unsigned Last, RegisterAccess Access>
    struct MsrField {
        static_assert(First < Last, "Invalid field range.");
        static_assert(Last < 64, "Invalid field range.");
    };
}
