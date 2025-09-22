#include "timer/hpet.hpp"
#include "log.hpp"
#include "logger/categories.hpp"
#include "memory/address_space.hpp"

// hpet id methods

mp::quantity<si::femto<si::second>, uint32_t> km::hpet::HpetId::refperiod() const {
    return uint32_t(mValue >> 32) * si::femto<si::second>;
}

pci::VendorId km::hpet::HpetId::vendorId() const {
    return pci::VendorId { uint16_t(mValue >> 16) };
}

bool km::hpet::HpetId::legacyRtCapable() const {
    return (mValue & (1 << 15)) != 0;
}

km::hpet::Width km::hpet::HpetId::counterSize() const {
    return (mValue & (1 << 13)) ? Width::QWORD : Width::DWORD;
}

uint8_t km::hpet::HpetId::timerCount() const {
    return uint8_t((mValue >> 8) & 0b11111) + 1;
}

uint8_t km::hpet::HpetId::revision() const {
    return uint8_t(mValue & 0xFF);
}

// hpet comparator methods

uint32_t km::hpet::Comparator::routeMask() const {
    return (mConfig & 0xFFFF'FFFF) >> 32;
}

bool km::hpet::Comparator::fsbIntDelivery() const {
    static constexpr auto kFsbIntDeliveryCap = 1 << 15;
    return (mConfig & kFsbIntDeliveryCap);
}

km::hpet::Width km::hpet::Comparator::width() const {
    static constexpr auto kWidthCap = 1 << 5;
    return (mConfig & kWidthCap) ? Width::QWORD : Width::DWORD;
}

bool km::hpet::Comparator::periodicSupport() const {
    static constexpr auto kPeriodicCap = 1 << 4;
    return (mConfig & kPeriodicCap);
}

uint64_t km::hpet::Comparator::counter() const {
    return mCounter;
}

km::hpet::ComparatorConfig km::hpet::Comparator::config() const {
    uint64_t config = mConfig;
    bool enabled = (config & (1 << 2)) != 0;
    apic::Trigger trigger = (config & (1 << 1)) ? apic::Trigger::eLevel : apic::Trigger::eEdge;
    bool periodic = (config & (1 << 3)) != 0;
    Width width = (config & (1 << 8)) ? Width::DWORD : Width::QWORD;
    uint8_t ioApicRoute = (config >> 8) & 0b1111;

    return ComparatorConfig {
        .ioApicRoute = ioApicRoute,
        .mode = width,
        .enable = enabled,
        .trigger = trigger,
        .periodic = periodic,
        .period = mCounter,
    };
}

void km::hpet::Comparator::configure(ComparatorConfig config) {
    uint64_t newConfig = mConfig;
    if (config.enable) {
        newConfig |= (1 << 2);
    } else {
        newConfig &= ~(1 << 2);
    }

    if (config.trigger == apic::Trigger::eEdge) {
        newConfig &= ~(1 << 1);
    } else {
        newConfig |= (1 << 1);
    }

    if (config.periodic) {
        newConfig |= (1 << 3);
    } else {
        newConfig &= ~(1 << 3);
    }

    if (config.mode == Width::DWORD) {
        newConfig |= (1 << 8);
    } else {
        newConfig &= ~(1 << 8);
    }

    newConfig &= ~(0b0011'1110'0000'0000);
    newConfig |= (config.ioApicRoute << 8);

    mConfig = newConfig;
}

// hpet methods

km::HighPrecisionTimer::HighPrecisionTimer(const acpi::Hpet *hpet, VmemAllocation allocation, hpet::MmioRegisters *mmio)
    : mTable(*hpet)
    , mAllocation(allocation)
    , mMmioRegion(mmio)
    , mId(mMmioRegion->id)
{ }

km::hertz km::HighPrecisionTimer::refclk() const {
    // TODO: Ideally I would use the femtoseconds value and do proper
    // conversions using mp_units. Currently the handling of femtoseconds
    // and frequency conversions doesn't make that possible.
    uint32_t femtos = (mMmioRegion->id >> 32);
    return (femtos * 10) * si::hertz;
}

km::hertz km::HighPrecisionTimer::frequency() const {
    return refclk();
}

uint64_t km::HighPrecisionTimer::ticks() const {
    //
    // The hpet counter can be either 32 or 64 bits wide, if it is 32 bits wide
    // the specification states that the top 32 bits will always be 0 on a 64 bit
    // read. This means that we can safely read the counter as a 64 bit value and
    // return it.
    //
    return mMmioRegion->masterCounter;
}

pci::VendorId km::HighPrecisionTimer::vendor() const {
    return mId.vendorId();
}

km::hpet::Width km::HighPrecisionTimer::counterSize() const {
    return mId.counterSize();
}

uint8_t km::HighPrecisionTimer::timerCount() const {
    return mId.timerCount();
}

uint8_t km::HighPrecisionTimer::revision() const {
    return mId.revision();
}

void km::HighPrecisionTimer::enable(bool enabled) {
    uint64_t config = mMmioRegion->config;

    if (enabled) {
        config |= 0x1;
    } else {
        config &= ~0x1;
    }

    mMmioRegion->config = config;
}

bool km::HighPrecisionTimer::isTimerActive(uint8_t timer) const {
    return (mMmioRegion->irqStatus & (1 << timer)) != 0;
}

std::span<km::hpet::Comparator> km::HighPrecisionTimer::comparators() {
    hpet::Comparator *begin = mMmioRegion->comparators;
    hpet::Comparator *end = begin + mId.timerCount();
    return std::span<hpet::Comparator>(begin, end);
}

std::span<const km::hpet::Comparator> km::HighPrecisionTimer::comparators() const {
    const hpet::Comparator *begin = mMmioRegion->comparators;
    const hpet::Comparator *end = begin + mId.timerCount();
    return std::span<const hpet::Comparator>(begin, end);
}

OsStatus km::setupHpet(const acpi::AcpiTables& rsdt, AddressSpace& memory, HighPrecisionTimer *timer) {
    for (const acpi::RsdtHeader *header : rsdt.entries()) {
        const acpi::Hpet *hpet = acpi::tableCast<acpi::Hpet>(header);
        if (hpet == nullptr)
            continue;

        acpi::GenericAddress baseAddress = hpet->baseAddress;
        if (baseAddress.addressSpace != acpi::AddressSpaceId::eSystemMemory) {
            AcpiLog.warnf("HPET base address (", baseAddress, ") is not in system memory. Currently unsupported.");
            return OsStatusInvalidAddress;
        }

        VmemAllocation allocation;
        hpet::MmioRegisters *mmio = memory.mapMmio<hpet::MmioRegisters>(km::PhysicalAddressEx { baseAddress.address }, PageFlags::eData, &allocation);
        if (mmio == nullptr) {
            AcpiLog.warnf("Failed to map HPET MMIO region.");
            return OsStatusOutOfMemory;
        }

        *timer = HighPrecisionTimer { hpet, allocation, mmio };
        return OsStatusSuccess;
    }

    AcpiLog.warnf("No HPET table found.");
    return OsStatusNotFound;
}
