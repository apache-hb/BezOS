#pragma once

#include "apic.hpp"

namespace km {
    enum class CpuCoreId : uint32_t {
        eInvalid = 0xFFFF'FFFF
    };

    void InitKernelThread(IntController pic);

    CpuCoreId GetCurrentCoreId();
    IntController GetCurrentCoreIntController();
}
