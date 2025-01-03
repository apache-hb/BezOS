#pragma once

#include "util/format.hpp"
#include <stdint.h>

namespace x64 {
    class Cr0 {
        uint64_t mValue;

        Cr0(uint64_t value) : mValue(value) { }

    public:
        enum : uint32_t {
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

        bool test(uint32_t flag) const {
            return mValue & flag;
        }

        void set(uint32_t flag) {
            mValue |= flag;
        }

        void clear(uint32_t flag) {
            mValue &= ~flag;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static Cr0 load() {
            uint64_t value;
            asm volatile("mov %%cr0, %0" : "=r"(value));
            return Cr0(value);
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(Cr0 cr0) {
            asm volatile("mov %0, %%cr0" : : "r"(cr0.mValue));
        }
    };
}

template<>
struct km::StaticFormat<x64::Cr0> {
    using String = stdx::StaticString<64>;
    static String toString(x64::Cr0 value);
};
