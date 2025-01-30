#include "pit.hpp"
#include "delay.hpp"
#include "isr.hpp"

#include <cstdint>

static constexpr uint16_t kChannel0 = 0x40;
// static constexpr uint16_t kChannel1 = 0x41;
// static constexpr uint16_t kChannel2 = 0x42;
static constexpr uint16_t kCommand = 0x43;

void km::IntervalTimer::setDivisor(uint16_t divisor) {
    IntGuard guard;

    KmWriteByte(kCommand, 0b00110100);

    KmWriteByte(kChannel0, divisor & 0xFF);
    KmWriteByte(kChannel0, (divisor >> 8) & 0xFF);
}

uint16_t km::IntervalTimer::bestDivisor(hertz frequency) const {
    return uint16_t(refclk() / frequency);
}

uint16_t km::IntervalTimer::getCount() const {
    IntGuard guard;

    KmWriteByte(kCommand, 0b00000000);

    uint8_t lo = KmReadByte(kChannel0);
    uint8_t hi = KmReadByte(kChannel0);

    return (hi << 8) | lo;
}

void km::IntervalTimer::setCount(uint16_t value) {
    IntGuard guard;

    KmWriteByte(kChannel0, value & 0xFF);
    KmWriteByte(kChannel0, (value >> 8) & 0xFF);
}

void km::InitPit(hertz frequency, const acpi::Madt *madt, IoApic& ioApic, IApic *apic, uint8_t irq, IsrCallback callback) {
    IntervalTimer pit;
    pit.setFrequency(frequency);

    apic::IvtConfig config {
        .vector = irq,
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
    };

    InstallIsrHandler(irq, callback);

    ioApic.setLegacyRedirect(config, irq::kTimer, madt, apic);
}
