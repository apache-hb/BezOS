#pragma once

#include "util/format.hpp"
#include <stdint.h>

namespace x64 {
    class Cr4 {
        uint64_t mValue;

        Cr4(uint64_t value) : mValue(value) { }

    public:
        enum : uint32_t {
            VME = (1ull << 0),
            PVI = (1ull << 1),
            TSD = (1ull << 2),
            DE = (1ull << 3),
            PSE = (1ull << 4),
            PAE = (1ull << 5),
            MCE = (1ull << 6),
            PGE = (1ull << 7),
            PCE = (1ull << 8),
            OSFXSR = (1ull << 9),
            OSXMMEXCPT = (1ull << 10),
            UMIP = (1ull << 11),
            LA57 = (1ull << 12),
            VMXE = (1ull << 13),
            SMXE = (1ull << 14),
            FSGSBASE = (1ull << 16),
            PCIDE = (1ull << 17),
            OSXSAVE = (1ull << 18),
            KL = (1ull << 19),
            SMEP = (1ull << 20),
            SMAP = (1ull << 21),
            PKE = (1ull << 22),
        };

        uint32_t value() const { return mValue; }

        static Cr4 of(uint32_t value) {
            return Cr4(value);
        }

        bool test(uint32_t flag) const {
            return mValue & flag;
        }

        void set(uint32_t flag) {
            mValue |= flag;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static Cr4 load() {
            uint64_t value;
            asm volatile("mov %%cr4, %0" : "=r"(value));
            return Cr4(value);
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(Cr4 cr4) {
            asm volatile("mov %0, %%cr4" : : "r"(cr4.mValue));
        }
    };
}

template<>
struct km::StaticFormat<x64::Cr4> {
    using String = stdx::StaticString<128>;
    static String toString(x64::Cr4 value);
};
