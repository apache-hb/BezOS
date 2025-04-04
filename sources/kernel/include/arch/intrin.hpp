#pragma once

#include <stdint.h>
#include <stddef.h>

#include <x86intrin.h>

#if defined(__x86_64__)
#   include "arch/x86_64/intrin.hpp"
#elif defined(__sparcv9__)
#   include "arch/sparcv9/intrin.hpp"
#else
#   error "Unsupported architecture"
#endif

struct [[gnu::packed]] alignas(16) GDTR {
    uint16_t limit;
    uint64_t base;
};

struct [[gnu::packed]] alignas(16) IDTR {
    uint16_t limit;
    uint64_t base;
};

#define __DEFAULT_FN_ATTRS __attribute__((__always_inline__, __nodebug__, unused))

static inline unsigned char __DEFAULT_FN_ATTRS __inbyte(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline unsigned short __DEFAULT_FN_ATTRS __inword(unsigned short port) {
    unsigned short ret;
    asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline unsigned long __DEFAULT_FN_ATTRS __indword(unsigned short port) {
    unsigned long ret;
    asm volatile("inl %w1, %k0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void __DEFAULT_FN_ATTRS __outbyte(unsigned short port, unsigned char data) {
    asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

static inline void __DEFAULT_FN_ATTRS __outword(unsigned short port, unsigned short data) {
    asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
}

static inline void __DEFAULT_FN_ATTRS __outdword(unsigned short port, unsigned long data) {
    asm volatile("outl %k0, %w1" : : "a"(data), "Nd"(port));
}

[[nodiscard]]
static inline uint64_t __DEFAULT_FN_ATTRS __rdmsr(uint32_t msr) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void __DEFAULT_FN_ATTRS __wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value;
    uint32_t high = value >> 32;
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline void __DEFAULT_FN_ATTRS __ltr(uint16_t selector) {
    asm volatile("ltr %0" :: "r"(selector));
}

static inline void __DEFAULT_FN_ATTRS __nop(void) {
    arch::Intrin::nop();
}

static inline void __DEFAULT_FN_ATTRS __cli(void) {
    arch::Intrin::cli();
}

static inline void __DEFAULT_FN_ATTRS __sti(void) {
    arch::Intrin::sti();
}

static inline void __DEFAULT_FN_ATTRS __halt(void) {
    arch::Intrin::halt();
}

static inline void __DEFAULT_FN_ATTRS __swapgs(void) {
    asm volatile("swapgs");
}

[[nodiscard]]
static inline uint64_t __DEFAULT_FN_ATTRS __gsbase(void) {
    uint64_t value;
    asm volatile("mov %%gs:0, %0" : "=r"(value));
    return value;
}

[[nodiscard]]
static inline uint64_t __DEFAULT_FN_ATTRS __fsbase(void) {
    uint64_t value;
    asm volatile("mov %%fs:0, %0" : "=r"(value));
    return value;
}

static inline __DEFAULT_FN_ATTRS void __invlpg(uintptr_t address) {
    arch::Intrin::invlpg(address);
}

static inline void __DEFAULT_FN_ATTRS __lgdt(struct GDTR gdtr) {
    asm volatile("lgdt %0" :: "m"(gdtr));
}

static inline void __DEFAULT_FN_ATTRS __lidt(struct IDTR idtr) {
    asm volatile("lidt %0" :: "m"(idtr));
}

template<uint8_t N>
static inline void __DEFAULT_FN_ATTRS __int() {
    asm volatile("int %0" :: "N"(N));
}

[[nodiscard]]
static inline uint64_t __DEFAULT_FN_ATTRS __get_xcr0() {
    uint64_t value;
    asm volatile("xgetbv" : "=a"(value) : "c"(0));
    return value;
}

static inline void __DEFAULT_FN_ATTRS __set_xcr0(uint64_t value) {
    asm volatile("xsetbv" :: "a"(value), "c"(0) : "memory");
}

#define X64_CONTROL_REGISTER(name) \
    [[nodiscard]] \
    static inline uint64_t __DEFAULT_FN_ATTRS __get_##name() { \
        uint64_t value; \
        asm volatile("mov %%" #name ", %0" : "=r"(value)); \
        return value; \
    } \
    static inline void __DEFAULT_FN_ATTRS __set_##name(uint64_t value) { \
        asm volatile("mov %0, %%" #name :: "r"(value)); \
    }

X64_CONTROL_REGISTER(cr0)
X64_CONTROL_REGISTER(cr2)
X64_CONTROL_REGISTER(cr3)
X64_CONTROL_REGISTER(cr4)

X64_CONTROL_REGISTER(dr7)
X64_CONTROL_REGISTER(dr6)

X64_CONTROL_REGISTER(dr3)
X64_CONTROL_REGISTER(dr2)
X64_CONTROL_REGISTER(dr1)
X64_CONTROL_REGISTER(dr0)

#undef __DEFAULT_FN_ATTRS
