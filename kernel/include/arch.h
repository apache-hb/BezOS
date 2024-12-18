// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <stdint.h>
#include <stddef.h>

// these are partially copied from clangs intrin.h, i cant pull in the full file
// as it uses microsoft types. although i could enable msvc extensions...

struct [[gnu::packed]] GDTR {
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

static inline void __DEFAULT_FN_ATTRS __nop(void) {
    asm volatile("nop");
}

static inline void __DEFAULT_FN_ATTRS __cli(void) {
    asm volatile("cli");
}

static inline void __DEFAULT_FN_ATTRS __sti(void) {
    asm volatile("sti");
}

static inline void __DEFAULT_FN_ATTRS __halt(void) {
    asm volatile("hlt");
}

static inline void __DEFAULT_FN_ATTRS __lgdt(struct GDTR gdtr) {
    asm volatile("lgdt %0" :: "m"(gdtr));
}
