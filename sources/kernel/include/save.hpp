#pragma once

#include "arch/msr.hpp"

#include <stdint.h>

static constexpr x64::RwModelRegister<0x0DA0> IA32_XSS;

namespace km {
    enum class XSaveFeature {
        FP = 0,
        SSE = 1,

        YMM = 2,
        BNDREGS = 3,
        BNDCSR = 4,
        OPMASK = 5,
        ZMM_HI256 = 6,
        HI16_ZMM = 7,
        PKRU = 9,
        PASID = 10,
        CET = 11,
        LBR = 15,
        XTILECFG = 17,
        XTILEDATA = 18,
    };

    struct XSaveSystemInfo {

    };

    class SaveManager {
        uint32_t mBufferSize;

    public:
        SaveManager();
    };
}
