#include "util/cpuid.hpp"

#include <string.h>
#include <cpuid.h>

km::CpuId km::CpuId::of(int leaf) {
    CpuId result{};
    __cpuid(leaf, result.eax, result.ebx, result.ecx, result.edx);
    return result;
}

km::CpuId km::CpuId::count(int leaf, int subleaf) {
    CpuId result{};
    __cpuid_count(leaf, subleaf, result.eax, result.ebx, result.ecx, result.edx);
    return result;
}

static void copyString(km::CpuId cpuid, char dst[16]) {
    memcpy(dst, cpuid.ureg, sizeof(cpuid.ureg));
}

void km::KmGetBrandString(char dst[kBrandStringSize]) {
    copyString(CpuId::of(0x80000002), dst + 0);
    copyString(CpuId::of(0x80000003), dst + 16);
    copyString(CpuId::of(0x80000004), dst + 32);
}
