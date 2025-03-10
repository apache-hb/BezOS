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
    return name == "KVMKVMKVM\0\0\0";
}

bool km::HypervisorInfo::isQemu() const {
    return name == "TCGTCGTCGTCG";
}

bool km::HypervisorInfo::isMicrosoftHyperV() const {
    return name == "Microsoft Hv";
}

bool km::HypervisorInfo::platformHasDebugPort() const {
    // qemu may use kvm, so check the e9 port in either case
    return isQemu() || isKvm();
}

std::optional<km::HypervisorInfo> km::GetHypervisorInfo() {
    if (!IsHypervisorPresent()) {
        return std::nullopt;
    }

    CpuId cpuid = CpuId::of(0x40000000);

    char vendor[12];
    memcpy(vendor + 0, &cpuid.ebx, 4);
    memcpy(vendor + 4, &cpuid.ecx, 4);
    memcpy(vendor + 8, &cpuid.edx, 4);

    return HypervisorInfo { std::to_array(vendor), cpuid.eax };
}

bool km::ProcessorInfo::isKvm() const {
    return vendor == "KVMKVMKVM\0\0\0"_sv;
}

km::BrandString km::GetBrandString() {
    char brand[sm::kBrandStringSize];
    sm::GetBrandString(brand);
    return std::to_array(brand);
}

km::ProcessorInfo km::GetProcessorInfo() {
    CpuId vendorId = CpuId::of(0x0);

    char vendor[12];
    memcpy(vendor + 0, &vendorId.ebx, 4);
    memcpy(vendor + 4, &vendorId.edx, 4);
    memcpy(vendor + 8, &vendorId.ecx, 4);
    uint32_t maxleaf = vendorId.eax;

    CpuId cpuid = CpuId::of(0x1);

    CpuId ext = CpuId::of(0x80000000);
    BrandString brand = ext.eax < 0x80000004 ? "" : GetBrandString();

    uintptr_t maxvaddr = 0;
    uintptr_t maxpaddr = 0;
    if (ext.eax >= 0x80000008) {
        CpuId leaf = CpuId::of(0x80000008);
        maxpaddr = (leaf.eax >> 0) & 0xFF;
        maxvaddr = (leaf.eax >> 8) & 0xFF;
    }

    bool invariantTsc = false;
    if (maxleaf >= 0x6) {
        CpuId timer = CpuId::of(0x6);
        invariantTsc = timer.eax & (1 << 2);
    }

    CpuId leaf7{};
    if (maxleaf >= 0x7) {
        leaf7 = CpuId::of(0x7);
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
        .vendor = std::to_array(vendor),
        .brand = brand,
        .maxleaf = maxleaf,
        .maxpaddr = maxpaddr,
        .maxvaddr = maxvaddr,
        .invariantTsc = invariantTsc,
        .coreClock = coreClock,
        .busClock = busClock * si::hertz,

        .l1ecx = cpuid.ecx,
        .l1edx = cpuid.edx,

        .l7ebx = leaf7.ebx,
        .l7ecx = leaf7.ecx,
        .l7edx = leaf7.edx,

        .baseFrequency = baseFrequency * si::megahertz,
        .maxFrequency = maxFrequency * si::megahertz,
        .busFrequency = busFrequency * si::megahertz,
    };
}
