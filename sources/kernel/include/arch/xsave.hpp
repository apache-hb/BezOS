#pragma once

#include <stdint.h>
#include <xmmintrin.h>
#include <immintrin.h>

#include "util/util.hpp"

namespace x64 {
    enum class XSaveMask : uint64_t {
        FPU = (1 << 0),
        SSE = (1 << 1),
        AVX = (1 << 2),
        MPX = (0b11 << 3),
        AVX512 = (0b111 << 5),
        PKRU = (1 << 9),
        AMX = (0b11 << 17),
    };

    struct [[gnu::packed]] FpuRegister {
        uint8_t reg[10];
        uint8_t reserved[6];
    };

    static_assert(sizeof(FpuRegister) == 16);

    struct [[gnu::packed]] alignas(64) FxSave {
        uint16_t fcw;
        uint16_t fsw;
        uint8_t ftw;
        uint8_t reserved0[1];
        uint16_t fop;
        uint32_t fip0;
        uint16_t fcs;
        uint16_t fip1;
        uint32_t fdp0;
        uint16_t fds;
        uint16_t fdp1;
        uint32_t mxcsr;
        uint32_t mxcsr_mask;

        FpuRegister fpu[8];
        __v4su xmm[16];

        uint8_t reserved1[96];
    };

    static_assert(sizeof(FxSave) == 512);

    struct [[gnu::packed]] alignas(64) XSaveHeader {
        uint64_t xstate_bv;
        uint64_t xcomp_bv;
        uint8_t reserved[48];
    };

    static_assert(sizeof(XSaveHeader) == 64);
}

static inline void __xsave(x64::FxSave *buffer, uint64_t mask) {
    _xsave64(buffer, mask);
}

static inline void __xrstor(x64::FxSave *buffer, uint64_t mask) {
    _xrstor64(buffer, mask);
}

UTIL_BITFLAGS(x64::XSaveMask);
