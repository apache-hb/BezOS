#pragma once

#include <stdint.h>

namespace sm {
    union CpuId {
        uint32_t ureg[4];
        int32_t ireg[4];
        struct { uint32_t eax; uint32_t ebx; uint32_t ecx; uint32_t edx; };

        static CpuId of(int leaf);
        static CpuId count(int leaf, int subleaf);
    };

    static constexpr unsigned kBrandStringSize = 3 * 4 * 4;
    static constexpr unsigned kVendorStringSize = 12;
    void GetBrandString(char dst[kBrandStringSize]);
    void GetVendorString(char dst[kVendorStringSize]);
    void GetHypervisorString(char dst[kVendorStringSize]);
}
