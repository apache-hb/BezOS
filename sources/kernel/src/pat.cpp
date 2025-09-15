#include "pat.hpp"

#include "util/cpuid.hpp"

#include "arch/msr.hpp"

static constexpr x64::RwModelRegister<0x277> IA32_PAT;

static constexpr x64::RoModelRegister<0xFE> IA32_MTRRCAP;
static constexpr x64::RwModelRegister<0x2FF> IA32_MTRR_DEF_TYPE;

static constexpr uint32_t kMtrrBaseMsrStart = 0x200;
static constexpr uint32_t kMtrrMaskMsrStart = 0x201;

static constexpr uint32_t kFixedRangeSupportBit = (1 << 8);
static constexpr uint32_t kFixedRangeEnabledBit = (1 << 10);
static constexpr uint32_t kWriteCombiningBit = (1 << 10);
static constexpr uint32_t kMtrrEnabledBit = (1 << 11);
static constexpr uint64_t kValidBit = (1 << 11);
static constexpr uint32_t kDefaultTypeMask = 0xFF;

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

static constexpr uint32_t kPatSupportBit = (1 << 16);
static constexpr uint32_t kMtrrSupportBit = (1 << 12);

/// feature testing

bool x64::hasPatSupport() {
    sm::CpuId cpuid = sm::CpuId::of(1);

    return cpuid.edx & kPatSupportBit;
}

bool x64::hasMtrrSupport() {
    sm::CpuId cpuid = sm::CpuId::of(1);

    return cpuid.edx & kMtrrSupportBit;
}

uint64_t x64::loadPatMsr(void) {
    return IA32_PAT.load();
}

/// PAT


void x64::detail::setPatEntry(uint64_t& pat, uint8_t index, km::MemoryType type) {
    pat &= ~(0xFFull << (index * 8));
    pat |= (uint64_t)type << (index * 8);
}

km::MemoryType x64::detail::getPatEntry(uint64_t pat, uint8_t index) {
    return km::MemoryType((pat >> (index * 8)) & 0xFF);
}

x64::PageAttributeTable::PageAttributeTable()
    : mValue(loadPatMsr())
{ }

void x64::PageAttributeTable::setEntry(uint8_t index, km::MemoryType type) {
    IA32_PAT.update(mValue, [&](uint64_t& value) {
        detail::setPatEntry(value, index, type);
    });
}

km::MemoryType x64::PageAttributeTable::getEntry(uint8_t index) const {
    return detail::getPatEntry(mValue, index);
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
    return mMtrrCap & kFixedRangeSupportBit;
}

bool x64::MemoryTypeRanges::fixedMtrrEnabled() const {
    return mMtrrDefault & kFixedRangeEnabledBit;
}

void x64::MemoryTypeRanges::enableFixedMtrrs(bool enabled) {
    IA32_MTRR_DEF_TYPE.updateBits(mMtrrDefault, kFixedRangeEnabledBit, enabled);
}

bool x64::MemoryTypeRanges::hasWriteCombining() const {
    return mMtrrCap & kWriteCombiningBit;
}

bool x64::MemoryTypeRanges::enabled() const {
    return mMtrrDefault & kMtrrEnabledBit;
}

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
    uint64_t msr = arch::IntrinX86_64::rdmsr(kFixedMtrrMsrs[index / 11]);
    uint64_t shift = (index % 11) * 8;
    uint64_t value = (msr >> shift) & 0xFF;
    return km::MemoryType(value);
}

void x64::MemoryTypeRanges::setFixedMtrr(uint8_t index, km::MemoryType mtrr) {
    uint64_t msr = arch::IntrinX86_64::rdmsr(kFixedMtrrMsrs[index / 11]);
    uint64_t shift = (index % 11) * 8;
    uint64_t mask = 0xFFull << shift;
    uint64_t value = (uint64_t)mtrr << shift;

    arch::IntrinX86_64::wrmsr(kFixedMtrrMsrs[index / 11], (msr & ~mask) | value);
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

    return VariableMtrr(arch::IntrinX86_64::rdmsr(base), arch::IntrinX86_64::rdmsr(mask));
}

void x64::MemoryTypeRanges::setVariableMtrr(uint8_t index, const km::PageBuilder& pm, km::MemoryType type, km::PhysicalAddress base, uintptr_t mask, bool enable) {
    uint32_t baseMsr = kMtrrBaseMsrStart + (index * 2);
    uint32_t maskMsr = kMtrrMaskMsrStart + (index * 2);

    uint64_t baseValue = (base.address & pm.getAddressMask()) | ((uint64_t)type & 0xFF);
    uint64_t maskValue = mask | (enable ? (1 << 11) : 0);

    arch::IntrinX86_64::wrmsr(baseMsr, baseValue);
    arch::IntrinX86_64::wrmsr(maskMsr, maskValue);
}
