#include "timer/pit.hpp"
#include "delay.hpp"
#include "isr/isr.hpp"
#include "apic.hpp"

#include <stdint.h>

using namespace std::chrono_literals;

static constexpr uint16_t kChannel0 = 0x40;
// static constexpr uint16_t kChannel1 = 0x41;
// static constexpr uint16_t kChannel2 = 0x42;
static constexpr uint16_t kCommand = 0x43;

void km::IntervalTimer::setDivisor(uint16_t divisor) {
    IntGuard guard;

    KmWriteByte(kCommand, 0b00110100);

    KmWriteByte(kChannel0, divisor & 0xFF);
    KmWriteByte(kChannel0, (divisor >> 8) & 0xFF);

    mDivisor = divisor;
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

km::IntervalTimer km::InitPit(hertz frequency, IoApicSet& ioApicSet, IApic *apic, uint8_t irq) {
    IntervalTimer pit;
    pit.setFrequency(frequency);

    apic::IvtConfig config {
        .vector = irq,
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
    };

    ioApicSet.setLegacyRedirect(config, irq::kTimer, apic);

    return pit;
}

void km::BusySleep(ITickSource *timer, std::chrono::microseconds duration) {
    auto frequency = timer->frequency();
    uint64_t ticks = (uint64_t(frequency / si::hertz) * duration.count()) / 1'000'000;
    uint64_t now = timer->ticks();
    uint64_t then = now + ticks;
    while (timer->ticks() < then) {
        _mm_pause();
    }
}
