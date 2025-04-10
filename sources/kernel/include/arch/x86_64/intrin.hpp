#pragma once

#include "arch/generic/intrin.hpp"

namespace arch {
    struct IntrinX86_64 : GenericIntrin {
        [[gnu::always_inline, gnu::nodebug]]
        static void nop() noexcept {
            asm volatile("nop");
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void halt() noexcept {
            asm volatile("hlt");
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void cli() noexcept {
            asm volatile("cli");
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void sti() noexcept {
            asm volatile("sti");
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void invlpg(uintptr_t address) noexcept {
            asm volatile("invlpg (%0)" : : "r"(address) : "memory");
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

        [[gnu::always_inline, gnu::nodebug]]
        static void outbyte(uint16_t port, uint8_t data) noexcept {
            asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug]]
        static uint8_t inbyte(uint16_t port) noexcept {
            uint8_t ret;
            asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void outword(uint16_t port, uint16_t data) noexcept {
            asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug]]
        static uint16_t inword(uint16_t port) noexcept {
            uint16_t ret;
            asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void outdword(uint16_t port, uint32_t data) noexcept {
            asm volatile("outl %k0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug]]
        static uint32_t indword(uint16_t port) noexcept {
            uint32_t ret;
            asm volatile("inl %w1, %k0" : "=a"(ret) : "Nd"(port));
            return ret;
        }
    };

    using Intrin = IntrinX86_64;
}
