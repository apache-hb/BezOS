#pragma once

#include "arch/generic/intrin.hpp"

namespace arch {
    struct IntrinSparcV9 : GenericIntrin {
        [[gnu::always_inline, gnu::nodebug]]
        static void nop() noexcept {
            asm volatile("nop");
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void halt() noexcept {
            /* TODO: not sure what sparc has that functions likes hlt on x86 */
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void cli() noexcept {
            asm volatile(
                "wrpr %0, %%pil"
                :
                : "i"(0xe)
                : "memory"
            );
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void sti() noexcept {
            asm volatile(
                "wrpr 0, %%pil"
                ::: "memory"
            );
        }
    };

    using Intrin = IntrinSparcV9;
}
