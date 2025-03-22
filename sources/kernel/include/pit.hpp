#pragma once

#include "apic.hpp"

#include "units.hpp"

#include <cstdint>

namespace km {
    class IApic;
    class HighPrecisionTimer;

    using hertz = mp::quantity<si::hertz, int64_t>;

    namespace pit {
        enum class Type {
            PIT8254,
            HPET,
            LAPIC,
            TSC,
        };
    }

    class ITickSource {
    public:
        virtual ~ITickSource() = default;

        virtual pit::Type type() const = 0;
        virtual hertz refclk() const = 0;
        virtual hertz frequency() const = 0;
        virtual uint64_t ticks() const = 0;
    };

    class IntervalTimer final : public ITickSource {
        uint16_t mDivisor = 1;

    public:
        static constexpr hertz kFrequencyHz = 1'193182 * si::hertz;

        pit::Type type() const override { return pit::Type::PIT8254; }
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

    IntervalTimer InitPit(hertz frequency, IoApicSet& ioApicSet, IApic *apic, uint8_t irq);

    void BusySleep(ITickSource *timer, std::chrono::microseconds duration);

    hertz TrainApicTimer(IApic *apic, ITickSource *refclk);
    hertz TrainInvariantTsc(ITickSource *refclk);
}
