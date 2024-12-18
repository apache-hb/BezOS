#pragma once

#include <stdint.h>

namespace km {
    union CpuId {
        uint32_t ureg[4];
        int32_t ireg[4];
        struct { uint32_t eax; uint32_t ebx; uint32_t ecx; uint32_t edx; };

        static CpuId of(int leaf) noexcept;
        static CpuId count(int leaf, int subleaf) noexcept;
    };

    static constexpr unsigned kBrandStringSize = 3 * 4 * 4;
    bool GetBrandString(char dst[kBrandStringSize]) noexcept;
}
