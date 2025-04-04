#pragma once

#include <stdint.h>

namespace arch {
    struct GenericIntrin {
        /// @brief No operation. Does nothing.
        [[gnu::error("nop not implemented by platform")]]
        static void nop() noexcept;

        /// @brief Halt the CPU until the next interrupt.
        [[gnu::error("hlt not implemented by platform")]]
        static void halt() noexcept;

        /// @brief Disable interrupts.
        [[gnu::error("cli not implemented by platform")]]
        static void cli() noexcept;

        /// @brief Enable interrupts.
        [[gnu::error("sti not implemented by platform")]]
        static void sti() noexcept;

        /// @brief Invalidate the TLB entry for the given address.
        [[gnu::error("invlpg not implemented by platform")]]
        static void invlpg(uintptr_t address) noexcept;

        /// @brief Read from a model-specific register (MSR).
        [[gnu::error("rdmsr not implemented by platform"), nodiscard]]
        static uint64_t rdmsr(uint32_t msr) noexcept;

        /// @brief Write to a model-specific register (MSR).
        [[gnu::error("wrmsr not implemented by platform")]]
        static void wrmsr(uint32_t msr, uint64_t value) noexcept;
    };
}
