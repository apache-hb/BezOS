#pragma once

#include "arch/generic/intrin.hpp"

namespace arch {
    using reg_t = uintptr_t;

    struct [[gnu::packed]] alignas(16) GDTR {
        uint16_t limit;
        uint64_t base;
    };

    struct [[gnu::packed]] alignas(16) IDTR {
        uint16_t limit;
        uint64_t base;
    };

    struct MachineStateX86_64 {
        reg_t rax;
        reg_t rbx;
        reg_t rcx;
        reg_t rdx;
        reg_t rdi;
        reg_t rsi;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
        reg_t r12;
        reg_t r13;
        reg_t r14;
        reg_t r15;
        reg_t rbp;
        reg_t rsp;
        reg_t rip;
        reg_t rflags;

        reg_t cs;
        reg_t ss;
    };

    struct LongJumpStateX86_64 {
        reg_t rbx;
        reg_t rbp;
        reg_t r12;
        reg_t r13;
        reg_t r14;
        reg_t r15;

        reg_t rsp;
        reg_t rip;
    };

    struct IntrinX86_64 : GenericIntrin {
        using MachineState = MachineStateX86_64;
        using LongJumpState = LongJumpStateX86_64;

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void nop() noexcept {
            asm volatile("nop");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void halt() noexcept {
            asm volatile("hlt");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void cli() noexcept {
            asm volatile("cli");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void sti() noexcept {
            asm volatile("sti");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void invlpg(uintptr_t address) noexcept {
            asm volatile("invlpg (%0)" : : "r"(address) : "memory");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void swapgs() noexcept {
            asm volatile("swapgs");
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void lgdt(const GDTR gdtr) noexcept {
            asm volatile("lgdt %0" : : "m"(gdtr));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void lidt(const IDTR idtr) noexcept {
            asm volatile("lidt %0" : : "m"(idtr));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void ltr(uint16_t selector) noexcept {
            asm volatile("ltr %0" : : "r"(selector));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used, nodiscard]]
        static uint64_t rdmsr(uint32_t msr) noexcept {
            uint32_t lo, hi;
            asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
            return ((uint64_t)hi << 32) | lo;
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void wrmsr(uint32_t msr, uint64_t value) noexcept [[clang::reentrant]] {
            uint32_t lo = value;
            uint32_t hi = value >> 32;
            asm volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void outbyte(uint16_t port, uint8_t data) noexcept [[clang::blocking, clang::nonallocating]] {
            asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static uint8_t inbyte(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
            uint8_t ret;
            asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void outword(uint16_t port, uint16_t data) noexcept [[clang::blocking, clang::nonallocating]] {
            asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static uint16_t inword(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
            uint16_t ret;
            asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static void outdword(uint16_t port, uint32_t data) noexcept [[clang::blocking, clang::nonallocating]] {
            asm volatile("outl %k0, %w1" : : "a"(data), "Nd"(port));
        }

        [[gnu::always_inline, gnu::nodebug, gnu::used]]
        static uint32_t indword(uint16_t port) noexcept [[clang::blocking, clang::nonallocating]] {
            uint32_t ret;
            asm volatile("inl %w1, %k0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        [[gnu::always_inline, gnu::nodebug, gnu::returns_twice, gnu::nonnull(1), gnu::used]]
        static int64_t setjmp(LongJumpState *state) noexcept asm("__x86_64_setjmp");

        [[gnu::always_inline, gnu::nodebug, noreturn, gnu::nonnull(1), gnu::used]]
        static void longjmp(LongJumpState *state, int64_t value) noexcept asm("__x86_64_longjmp");
    };

    using Intrin = IntrinX86_64;
}
