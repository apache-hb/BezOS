#include "hypervisor.hpp"

#include "util/cpuid.hpp"

#include <string.h>

using sm::CpuId;
using namespace stdx::literals;

bool km::IsHypervisorPresent() {
    static constexpr uint32_t kHypervisorBit = (1 << 31);

    CpuId cpuid = CpuId::of(1);
    return cpuid.ecx & kHypervisorBit;
}

bool km::HypervisorInfo::isKvm() const {
    return name == "KVMKVMKVM\0\0\0"_sv;
}

bool km::HypervisorInfo::isQemu() const {
    return name == "TCGTCGTCGTCG"_sv;
}

bool km::HypervisorInfo::isMicrosoftHyperV() const {
    return name == "Microsoft Hv"_sv;
}

bool km::HypervisorInfo::platformHasDebugPort() const {
    // qemu may use kvm, so check the e9 port in either case
    return isQemu() || isKvm();
}

km::HypervisorInfo km::KmGetHypervisorInfo() {
    CpuId cpuid = CpuId::of(0x40000000);

    char vendor[12];
    memcpy(vendor + 0, &cpuid.ebx, 4);
    memcpy(vendor + 4, &cpuid.ecx, 4);
    memcpy(vendor + 8, &cpuid.edx, 4);

    return HypervisorInfo { vendor, cpuid.eax };
}

km::BrandString km::KmGetBrandString() {
    char brand[sm::kBrandStringSize];
    sm::KmGetBrandString(brand);
    return brand;
}

km::ProcessorInfo km::KmGetProcessorInfo() {
    CpuId vendorId = CpuId::of(0);

    char vendor[12];
    memcpy(vendor + 0, &vendorId.ebx, 4);
    memcpy(vendor + 4, &vendorId.edx, 4);
    memcpy(vendor + 8, &vendorId.ecx, 4);
    uint32_t maxleaf = vendorId.eax;

    CpuId cpuid = CpuId::of(1);
    bool isLocalApicPresent = cpuid.edx & (1 << 9);
    bool is2xApicPresent = cpuid.ecx & (1 << 21);

    CpuId ext = CpuId::of(0x80000000);
    BrandString brand = ext.eax < 0x80000004 ? "" : KmGetBrandString();

    uintptr_t maxvaddr = 0;
    uintptr_t maxpaddr = 0;
    if (ext.eax >= 0x80000008) {
        CpuId leaf = CpuId::of(0x80000008);
        maxpaddr = (leaf.eax >> 0) & 0xFF;
        maxvaddr = (leaf.eax >> 8) & 0xFF;
    }

    CoreMultiplier coreClock = { 0, 0 };
    uint32_t busClock = 0;
    uint16_t baseFrequency = 0;
    uint16_t maxFrequency = 0;
    uint16_t busFrequency = 0;

    if (maxleaf >= 0x15) {
        CpuId tsc = CpuId::of(0x15);

        coreClock = {
            .tsc = tsc.eax,
            .core = tsc.ebx
        };

        busClock = tsc.ecx;
    }

    if (maxleaf >= 0x16) {
        CpuId frequency = CpuId::of(0x16);
        baseFrequency = frequency.eax & 0xFFFF;
        maxFrequency = frequency.ebx & 0xFFFF;
        busFrequency = frequency.ecx & 0xFFFF;
    }

    return ProcessorInfo {
        .vendor = vendor,
        .brand = brand,
        .maxleaf = maxleaf,
        .maxpaddr = maxpaddr,
        .maxvaddr = maxvaddr,
        .hasLocalApic = isLocalApicPresent,
        .has2xApic = is2xApicPresent,
        .coreClock = coreClock,
        .busClock = busClock,
        .baseFrequency = baseFrequency,
        .maxFrequency = maxFrequency,
        .busFrequency = busFrequency
    };
}
