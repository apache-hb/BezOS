#include "pat.hpp"

#include "util/cpuid.hpp"

#include "arch/msr.hpp"

static constexpr x64::RwModelRegister<0x277> IA32_PAT;

static constexpr x64::RoModelRegister<0xFE> IA32_MTRRCAP;
static constexpr x64::RwModelRegister<0x2FF> IA32_MTRR_DEF_TYPE;

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

uint64_t x64::LoadPatMsr(void) {
    return IA32_PAT.load();
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
    : mValue(LoadPatMsr())
{ }

void x64::PageAttributeTable::setEntry(uint8_t index, km::MemoryType type) {
    IA32_PAT.update(mValue, [&](uint64_t& mValue) {
        detail::SetPatEntry(mValue, index, type);
    });
}

km::MemoryType x64::PageAttributeTable::getEntry(uint8_t index) const {
    return detail::GetPatEntry(mValue, index);
}

/// MTRR

x64::MemoryTypeRanges::MemoryTypeRanges()
    : mMtrrCap(IA32_MTRRCAP.load())
    , mMtrrDefault(IA32_MTRR_DEF_TYPE.load())
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
    IA32_MTRR_DEF_TYPE.updateBits(mMtrrDefault, kFixedRangeEnabledBit, enabled);
}

bool x64::MemoryTypeRanges::hasWriteCombining() const {
    static constexpr uint32_t kWriteCombiningBit = (1 << 10);
    return mMtrrCap & kWriteCombiningBit;
}

static constexpr uint32_t kMtrrEnabledBit = (1 << 11);

bool x64::MemoryTypeRanges::enabled() const {
    return mMtrrDefault & kMtrrEnabledBit;
}

static constexpr uint32_t kDefaultTypeMask = 0xFF;

km::MemoryType x64::MemoryTypeRanges::defaultType() const {
    return km::MemoryType(mMtrrDefault & kDefaultTypeMask);
}

void x64::MemoryTypeRanges::setDefaultType(km::MemoryType type) {
    IA32_MTRR_DEF_TYPE.update(mMtrrDefault, [&](uint64_t& value) {
        value = (value & ~kDefaultTypeMask) | (uint8_t)type;
    });
}

void x64::MemoryTypeRanges::enable(bool enabled) {
    IA32_MTRR_DEF_TYPE.updateBits(mMtrrDefault, kMtrrEnabledBit, enabled);
}

/// fixed mtrrs

km::MemoryType x64::MemoryTypeRanges::fixedMtrr(uint8_t index) const {
    uint64_t msr = __rdmsr(kFixedMtrrMsrs[index / 11]);
    uint64_t shift = (index % 11) * 8;
    uint64_t value = (msr >> shift) & 0xFF;
    return km::MemoryType(value);
}

void x64::MemoryTypeRanges::setFixedMtrr(uint8_t index, km::MemoryType mtrr) {
    uint64_t msr = __rdmsr(kFixedMtrrMsrs[index / 11]);
    uint64_t shift = (index % 11) * 8;
    uint64_t mask = 0xFFull << shift;
    uint64_t value = (uint64_t)mtrr << shift;

    __wrmsr(kFixedMtrrMsrs[index / 11], (msr & ~mask) | value);
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

km::PhysicalAddress x64::VariableMtrr::baseAddress(const km::PageBuilder& pm) const {
    return km::PhysicalAddress { mBase & pm.getAddressMask() };
}

uintptr_t x64::VariableMtrr::addressMask(const km::PageBuilder& pm) const {
    return mMask & pm.getAddressMask();
}

x64::VariableMtrr x64::MemoryTypeRanges::variableMtrr(uint8_t index) const {
    uint32_t base = kMtrrBaseMsrStart + (index * 2);
    uint32_t mask = kMtrrMaskMsrStart + (index * 2);

    return VariableMtrr(__rdmsr(base), __rdmsr(mask));
}

void x64::MemoryTypeRanges::setVariableMtrr(uint8_t index, const km::PageBuilder& pm, km::MemoryType type, km::PhysicalAddress base, uintptr_t mask, bool enable) {
    uint32_t baseMsr = kMtrrBaseMsrStart + (index * 2);
    uint32_t maskMsr = kMtrrMaskMsrStart + (index * 2);

    uint64_t baseValue = (base.address & pm.getAddressMask()) | ((uint64_t)type & 0xFF);
    uint64_t maskValue = mask | (enable ? (1 << 11) : 0);

    __wrmsr(baseMsr, baseValue);
    __wrmsr(maskMsr, maskValue);
}
