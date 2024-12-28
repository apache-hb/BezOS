#include "pat.hpp"

#include "util/cpuid.hpp"

#include "msr.hpp"

static constexpr x64::ModelRegister<0x277, x64::RegisterAccess::eReadWrite> kPatMsr;

static constexpr x64::ModelRegister<0xFE, x64::RegisterAccess::eRead> kMtrrMsrCap;
static constexpr x64::ModelRegister<0x2FF, x64::RegisterAccess::eReadWrite> kMtrrMsrDefaultType;

static constexpr uint32_t kMtrrBaseMsrStart = 0x200;
static constexpr uint32_t kMtrrMaskMsrStart = 0x201;

static constexpr uint32_t kFixedMtrrMsrs[] = {
    0x250, // MTRR_FIX64K_00000
    0x258, // MTRR_FIX16K_80000
    0x259, // MTRR_FIX16K_A0000
    0x268, // MTRR_FIX4K_C0000
    0x269, // MTRR_FIX4K_C8000
    0x26a, // MTRR_FIX4K_D0000
    0x26b, // MTRR_FIX4K_D8000
    0x26c, // MTRR_FIX4K_E0000
    0x26d, // MTRR_FIX4K_E8000
    0x26e, // MTRR_FIX4K_F0000
    0x26f, // MTRR_FIX4K_F8000
};

/// feature testing

bool x64::HasPatSupport() {
    static constexpr uint32_t kPatSupportBit = (1 << 16);
    sm::CpuId cpuid = sm::CpuId::of(1);

    return cpuid.edx & kPatSupportBit;
}

bool x64::HasMtrrSupport() {
    static constexpr uint32_t kMtrrSupportBit = (1 << 12);
    sm::CpuId cpuid = sm::CpuId::of(1);

    return cpuid.edx & kMtrrSupportBit;
}

/// PAT


void x64::detail::SetPatEntry(uint64_t& pat, uint8_t index, km::MemoryType type) {
    pat &= ~(0xFFull << (index * 8));
    pat |= (uint64_t)type << (index * 8);
}

km::MemoryType x64::detail::GetPatEntry(uint64_t pat, uint8_t index) {
    return km::MemoryType((pat >> (index * 8)) & 0xFF);
}

x64::PageAttributeTable::PageAttributeTable()
    : mValue(kPatMsr.load())
{ }

void x64::PageAttributeTable::setEntry(uint8_t index, km::MemoryType type) {
    kPatMsr.update(mValue, [&](uint64_t& mValue) {
        detail::SetPatEntry(mValue, index, type);
    });
}

km::MemoryType x64::PageAttributeTable::getEntry(uint8_t index) const {
    return detail::GetPatEntry(mValue, index);
}

/// MTRR

x64::MemoryTypeRanges::MemoryTypeRanges()
    : mMtrrCap(kMtrrMsrCap.load())
    , mMtrrDefault(kMtrrMsrDefaultType.load())
{ }

uint8_t x64::MemoryTypeRanges::variableMtrrCount() const {
    return mMtrrCap & 0xFF;
}

bool x64::MemoryTypeRanges::fixedMtrrSupported() const {
    static constexpr uint32_t kFixedRangeSupportBit = (1 << 8);
    return mMtrrCap & kFixedRangeSupportBit;
}

static constexpr uint32_t kFixedRangeEnabledBit = (1 << 10);

bool x64::MemoryTypeRanges::fixedMtrrEnabled() const {
    return mMtrrDefault & kFixedRangeEnabledBit;
}

void x64::MemoryTypeRanges::enableFixedMtrrs(bool enabled) {
    kMtrrMsrDefaultType.updateBits(mMtrrDefault, kFixedRangeEnabledBit, enabled);
}

bool x64::MemoryTypeRanges::hasWriteCombining() const {
    static constexpr uint32_t kWriteCombiningBit = (1 << 10);
    return mMtrrCap & kWriteCombiningBit;
}

static constexpr uint32_t kMtrrEnabledBit = (1 << 11);

bool x64::MemoryTypeRanges::enabled() const {
    return mMtrrDefault & kMtrrEnabledBit;
}

void x64::MemoryTypeRanges::enable(bool enabled) {
    kMtrrMsrDefaultType.updateBits(mMtrrDefault, kMtrrEnabledBit, enabled);
}

/// fixed mtrrs

x64::FixedMtrr::FixedMtrr(uint8_t value)
    : mValue(value)
{ }

km::MemoryType x64::FixedMtrr::type() const {
    return km::MemoryType(mValue);
}

x64::FixedMtrr x64::MemoryTypeRanges::fixedMtrr(uint8_t index) const {
    uint64_t msr = __rdmsr(kFixedMtrrMsrs[index / 11]);
    uint64_t shift = (index % 11) * 8;
    uint64_t value = (msr >> shift) & 0xFF;
    return x64::MemoryTypeRanges::FixedMtrr(value);
}

/// variable mtrrs

x64::VariableMtrr::VariableMtrr(uint64_t base, uint64_t mask)
    : mBase(base)
    , mMask(mask)
{ }

km::MemoryType x64::VariableMtrr::type() const {
    return km::MemoryType(mBase & 0xFF);
}

bool x64::VariableMtrr::valid() const {
    static constexpr uint64_t kValidBit = (1 << 11);
    return mMask & kValidBit;
}

km::PhysicalAddress x64::VariableMtrr::baseAddress(const km::PageManager& pm) const {
    return km::PhysicalAddress { mBase & pm.getAddressMask() };
}

uintptr_t x64::VariableMtrr::addressMask(const km::PageManager& pm) const {
    return mMask & pm.getAddressMask();
}

x64::VariableMtrr x64::MemoryTypeRanges::variableMtrr(uint8_t index) const {
    uint32_t base = kMtrrBaseMsrStart + (index * 2);
    uint32_t mask = kMtrrMaskMsrStart + (index * 2);

    return VariableMtrr(__rdmsr(base), __rdmsr(mask));
}
