#pragma once

#include "arch/xsave.hpp"

#include <stdint.h>

namespace km {
    struct XSaveSystemInfo {
        uint32_t sizes[x64::eSaveCount];
    };

    uint32_t XSaveComponentSize(x64::XSaveFeature feature);

    class SaveManager {
        uint32_t mBufferSize;

    public:
        SaveManager();
    };
}
