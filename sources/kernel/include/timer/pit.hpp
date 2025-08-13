#pragma once

#include "timer/tick_source.hpp"

namespace km {
    class IApic;
    class IoApicSet;

    class IntervalTimer final : public ITickSource {
        uint16_t mDivisor = 1;

    public:
        static constexpr hertz kFrequencyHz = 1'193182 * si::hertz;

        TickSourceType type() const override { return TickSourceType::PIT8254; }
        hertz refclk() const override { return kFrequencyHz; }
        hertz frequency() const override { return kFrequencyHz / mDivisor; }
        uint64_t ticks() const override { return getCount(); }

        uint16_t bestDivisor(hertz frequency) const;
        void setDivisor(uint16_t divisor);

        void setFrequency(hertz frequency) {
            setDivisor(bestDivisor(frequency));
        }

        uint16_t getCount() const;
        void setCount(uint16_t count);
    };

    IntervalTimer setupPit(hertz frequency, IoApicSet& ioApicSet, IApic *apic, uint8_t irq);
}
