#include "pat.hpp"

#include "arch/intrin.hpp"
#include "util/cpuid.hpp"

static constexpr uint32_t kPatMsr = 0x277;
static constexpr uint32_t kMtrrMsrCap = 0xFE;
static constexpr uint32_t kMtrrMsrDefaultType = 0x2FF;

bool x64::HasPatSupport() {
    static constexpr uint32_t kPatSupportBit = (1 << 16);
    km::CpuId cpuid = km::CpuId::of(1);

    return cpuid.edx & kPatSupportBit;
}

bool x64::HasMtrrSupport() {
    static constexpr uint32_t kMtrrSupportBit = (1 << 12);
    km::CpuId cpuid = km::CpuId::of(1);

    return cpuid.edx & kMtrrSupportBit;
}

x64::PageAttributeTable::PageAttributeTable()
    : mValue(__rdmsr(kPatMsr))
{ }

void x64::PageAttributeTable::setEntry(uint8_t index, PageType type) {
    mValue &= ~(0xFFull << (index * 8));
    mValue |= (uint64_t)type << (index * 8);

    __wrmsr(kPatMsr, mValue);
}

x64::MemoryTypeRanges::MemoryTypeRanges()
    : mMtrrCap(__rdmsr(kMtrrMsrCap))
    , mMtrrDefault(__rdmsr(kMtrrMsrDefaultType))
{ }

uint8_t x64::MemoryTypeRanges::mtrrCount() const {
    return mMtrrCap & 0xFF;
}

bool x64::MemoryTypeRanges::hasWriteCombining() const {
    static constexpr uint32_t kWriteCombiningBit = (1 << 10);
    return mMtrrCap & kWriteCombiningBit;
}

bool x64::MemoryTypeRanges::fixedRangeMtrrEnabled() const {
    static constexpr uint32_t kFixedRangeBit = (1 << 10);
    return mMtrrDefault & kFixedRangeBit;
}

bool x64::MemoryTypeRanges::hasAllFixedRangeMtrrs() const {
    static constexpr uint32_t kAllFixedRangeBit = (1 << 8);
    return mMtrrCap & kAllFixedRangeBit;
}

bool x64::MemoryTypeRanges::enabled() const {
    static constexpr uint32_t kMtrrEnabledBit = (1 << 11);
    return mMtrrDefault & kMtrrEnabledBit;
}
