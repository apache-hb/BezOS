#pragma once

#include <stdint.h>

namespace arch {
    struct GenericIntrin {
        using MachineState = void;
        using FpuMachineState = void;
        using LongJumpState = void;

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

        [[gnu::error("outbyte not implemented by platform")]]
        static void outbyte(uint16_t port, uint8_t value) noexcept;

        [[gnu::error("inbyte not implemented by platform")]]
        static uint8_t inbyte(uint16_t port) noexcept;

        [[gnu::error("outword not implemented by platform")]]
        static void outword(uint16_t port, uint16_t value) noexcept;

        [[gnu::error("inword not implemented by platform")]]
        static uint16_t inword(uint16_t port) noexcept;

        [[gnu::error("outdword not implemented by platform")]]
        static void outdword(uint16_t port, uint32_t value) noexcept;

        [[gnu::error("indword not implemented by platform")]]
        static uint32_t indword(uint16_t port) noexcept;

        [[gnu::error("setjmp not implemented by platform"), gnu::returns_twice, gnu::nonnull(1)]]
        static int64_t setjmp(LongJumpState *state) noexcept;

        [[gnu::error("longjmp not implemented by platform"), noreturn, gnu::nonnull(1)]]
        static void longjmp(LongJumpState *state, int64_t value) noexcept;
    };
}
