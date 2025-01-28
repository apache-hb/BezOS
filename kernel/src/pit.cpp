#include "pit.hpp"
#include "delay.hpp"
#include "isr.hpp"
#include "log.hpp"
#include "processor.hpp"

#include <cstdint>

static constexpr uint16_t kChannel0 = 0x40;
// static constexpr uint16_t kChannel1 = 0x41;
// static constexpr uint16_t kChannel2 = 0x42;
static constexpr uint16_t kCommand = 0x43;

static uint16_t ComputePitReloadValue(unsigned frequency) {
    uint16_t result = km::Pit::kFrequencyHz / frequency;
    if ((km::Pit::kFrequencyHz % frequency) > (frequency / 2)) {
        result++;
    }
    return result;
}

uint16_t km::Pit::getCount() {
    IntGuard guard;

    KmWriteByte(kCommand, 0b00000000);

    uint8_t lo = KmReadByte(kChannel0);
    uint8_t hi = KmReadByte(kChannel0);

    return (hi << 8) | lo;
}

void km::Pit::setCount(uint16_t value) {
    IntGuard guard;

    KmWriteByte(kChannel0, value & 0xFF);
    KmWriteByte(kChannel0, (value >> 8) & 0xFF);
}

void km::Pit::setFrequency(unsigned frequency) {
    IntGuard guard;

    uint16_t divisor = kFrequencyHz / frequency;

    // Channel 0, lo/hi byte, rate generator
    KmWriteByte(kCommand, 0b00110100);

    KmWriteByte(kChannel0, divisor & 0xFF);
    KmWriteByte(kChannel0, (divisor >> 8) & 0xFF);
}

void km::InitPit(unsigned frequency, const acpi::Madt *madt, IoApic& ioApic, IApic *apic, IsrAllocator& isrAllocator) {
    Pit pit;
    pit.setFrequency(frequency);

    uint16_t now = pit.getCount();

    uint8_t irq = isrAllocator.allocateIsr();
    apic::IvtConfig config {
        .vector = irq,
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
    };

    InstallIsrHandler(irq, [](IsrContext *ctx) -> void* {
        KmDebugMessage("[PIT] Timer: ", ctx->vector, "\n");

        IApic *apic = GetCurrentCoreApic();
        apic->eoi();
        return ctx;
    });

    apic->cfgIvtTimer(config);

    ioApic.setLegacyRedirect(config, 0, madt, apic);

    uint16_t then = pit.getCount();

    KmDebugMessage("[PIT] Time between ticks: ", now, " ", then, "\n");
}
