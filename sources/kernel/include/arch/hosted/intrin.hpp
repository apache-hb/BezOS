#pragma once

#include "arch/generic/intrin.hpp"

#include <atomic>

namespace arch {
    struct HostedIntrin : GenericIntrin {
        [[gnu::always_inline, gnu::nodebug]]
        static void nop() noexcept {
            /* no-op on hosted platforms */
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void halt() noexcept {
            /* no-op on hosted platforms */
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void cli() noexcept {
            /* no-op on hosted platforms */
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void sti() noexcept {
            /* no-op on hosted platforms */
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void invlpg(uintptr_t) noexcept {
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }

        [[gnu::always_inline, gnu::nodebug, nodiscard]]
        static uint64_t rdmsr(uint32_t msr) noexcept {
            uint32_t lo, hi;
            asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
            return ((uint64_t)hi << 32) | lo;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void wrmsr(uint32_t msr, uint64_t value) noexcept {
            uint32_t lo = value;
            uint32_t hi = value >> 32;
            asm volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
        }
    };

    using Intrin = HostedIntrin;
}
