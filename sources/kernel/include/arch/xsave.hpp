#pragma once

#include <cstddef>
#include <stdint.h>
#include <xmmintrin.h>
#include <immintrin.h>

#include "arch/msr.hpp"
#include "common/util/util.hpp"
#include "common/compiler/compiler.hpp"

#include <concepts>

static constexpr x64::RwModelRegister<0x00000DA0> IA32_XSS;

namespace x64 {
    /// @brief XSAVE feature mask.
    ///
    /// Specified in Intel SDM Vol 1, Chapter 13.1.
    enum XSaveFeature {
        FPU = 0,
        SSE = 1,
        AVX = 2,

        BNDREG = 3,
        BNDCSR = 4,

        OPMASK = 5,
        ZMM_HI256 = 6,
        HI16_ZMM = 7,

        PT = 8,
        PKRU = 9,
        PASID = 10,

        CET_U = 11,
        CET_S = 12,

        HDC = 13,
        UINTR = 14,
        LBR = 15,
        HWP = 16,

        TILECFG = 17,
        TILEDATA = 18,

        APX = 19,

        eSaveCount = 20,
    };

    template<typename... A> requires ((std::same_as<A, XSaveFeature> && ...) && sizeof...(A) > 0)
    constexpr uint64_t XSaveMask(A&&... args) noexcept {
        return ((UINT64_C(1) << std::to_underlying(args)) | ...);
    }

    /// @brief XSAVE feature mask for MPX support.
    constexpr uint64_t kSaveMpx = XSaveMask(BNDREG, BNDCSR);

    /// @brief XSAVE feature mask for AVX-512 support.
    constexpr uint64_t kSaveAvx512 = XSaveMask(OPMASK, ZMM_HI256, HI16_ZMM);

    /// @brief XSAVE feature mask for CET support.
    constexpr uint64_t kSaveCet = XSaveMask(CET_U, CET_S);

    /// @brief XSAVE feature mask for AMX support.
    constexpr uint64_t kSaveAmx = XSaveMask(TILECFG, TILEDATA);

    /// @brief All XSAVE features that are configured in the XCR0 register.
    constexpr uint64_t kXcr0Mask = XSaveMask(
        FPU, SSE, AVX,
        BNDREG, BNDCSR,
        OPMASK, ZMM_HI256, HI16_ZMM,
        PKRU,
        TILECFG, TILEDATA,
        APX
    );

    /// @brief All XSAVE features that are configured in the IA32_XSS msr.
    constexpr uint64_t kXssMask = XSaveMask(
        PT,
        PASID,
        CET_U, CET_S,
        HDC,
        UINTR,
        LBR,
        HWP
    );

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

    struct [[gnu::packed]] alignas(64) XSave {
        FxSave fpu;
        XSaveHeader header;
        std::byte data[];
    };
}

CLANG_DIAGNOSTIC_PUSH();
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");

[[gnu::nodebug, gnu::always_inline]]
static inline void __fxsave(x64::FxSave *buffer) noexcept [[clang::reentrant]] {
    _fxsave(buffer);
}

[[gnu::nodebug, gnu::always_inline]]
static inline void __fxrstor(x64::FxSave *buffer) noexcept [[clang::reentrant]] {
    _fxrstor(buffer);
}

[[gnu::nodebug, gnu::always_inline, gnu::target("xsave")]]
static inline void __xsave(x64::XSave *buffer, uint64_t mask) noexcept [[clang::reentrant]] {
    _xsave64(buffer, mask);
}

[[gnu::nodebug, gnu::always_inline, gnu::target("xsave")]]
static inline void __xrstor(x64::XSave *buffer, uint64_t mask) noexcept [[clang::reentrant]] {
    _xrstor64(buffer, mask);
}

CLANG_DIAGNOSTIC_POP();

UTIL_BITFLAGS(x64::XSaveFeature);
