#include "log.hpp"
#include "pit.hpp"

km::HighPrecisionTimer::HighPrecisionTimer(const acpi::Hpet *hpet, SystemMemory& memory)
    : mTable(*hpet)
    , mMmioRegion(memory.mmioRegion<HpetRegisters>(km::PhysicalAddress { mTable.baseAddress.address }))
{ }

km::pit::Type km::HighPrecisionTimer::type() const { return pit::Type::HPET; }

uint16_t km::HighPrecisionTimer::bestDivisor(hertz) const {
    return 1; // TODO: Implement
}

km::hertz km::HighPrecisionTimer::refclk() const {
    // TODO: Ideally I would use the femtoseconds value and do proper
    // conversions using mp_units. Currently the handling of femtoseconds
    // and frequency conversions doesn't make that possible.
    uint32_t femtos = (mMmioRegion->id >> 32);
    return (femtos * 10) * si::hertz;
}

uint64_t km::HighPrecisionTimer::ticks() const {
    return 0;
}

void km::HighPrecisionTimer::setDivisor(uint16_t) {
    // TODO: Implement
}

pci::VendorId km::HighPrecisionTimer::vendor() const {
    return pci::VendorId { uint16_t(mMmioRegion->id >> 16) };
}

km::HpetWidth km::HighPrecisionTimer::counterSize() const {
    static constexpr uint64_t kCounterSize = 1 << 13;
    return (mMmioRegion->id & kCounterSize) ? HpetWidth::QWORD : HpetWidth::DWORD;
}

uint8_t km::HighPrecisionTimer::timerCount() const {
    return uint8_t((mMmioRegion->id >> 8) & 0b11111) + 1;
}

uint8_t km::HighPrecisionTimer::revision() const {
    return uint8_t(mMmioRegion->id & 0b11111111);
}

void km::HighPrecisionTimer::enable(bool enabled) {
    if (enabled) {
        mMmioRegion->config = (mMmioRegion->config & ~0b1) | 0b1;
    } else {
        mMmioRegion->config = mMmioRegion->config & ~0b1;
    }
}

bool km::HighPrecisionTimer::isTimerActive(uint8_t timer) const {
    return (mMmioRegion->irqStatus & (1 << timer)) != 0;
}

std::optional<km::HighPrecisionTimer> km::HighPrecisionTimer::find(const acpi::AcpiTables& acpiTables, SystemMemory& memory) {
    for (const acpi::RsdtHeader *header : acpiTables.entries()) {
        if (header->signature != acpi::Hpet::kSignature)
            continue;

        const acpi::Hpet *hpet = reinterpret_cast<const acpi::Hpet*>(header);
        if (hpet->baseAddress.addressSpace != acpi::AddressSpaceId::eSystemMemory) {
            KmDebugMessage("[WARN] HPET base address (", hpet->baseAddress, ") is not in system memory. Currently unsupported.\n");
            continue;
        }

        return HighPrecisionTimer { hpet, memory };
    }

    KmDebugMessage("[ACPI] No HPET table found.\n");
    return std::nullopt;
}
