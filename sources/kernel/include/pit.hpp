#pragma once

#include "apic.hpp"

#include "units.hpp"

#include <cstdint>

namespace km {
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
        virtual uint16_t bestDivisor(hertz frequency) const = 0;
        virtual uint64_t ticks() const = 0;
        virtual void setDivisor(uint16_t divisor) = 0;

        void setFrequency(hertz frequency) {
            setDivisor(bestDivisor(frequency));
        }
    };

    class IntervalTimer final : public ITickSource {
    public:
        static constexpr hertz kFrequencyHz = 1'193182 * si::hertz;

        pit::Type type() const override { return pit::Type::PIT8254; }
        hertz refclk() const override { return kFrequencyHz; }
        uint16_t bestDivisor(hertz frequency) const override;
        uint64_t ticks() const override { return getCount(); }
        void setDivisor(uint16_t divisor) override;

        uint16_t getCount() const;
        void setCount(uint16_t count);
    };

    void InitPit(hertz frequency, IoApicSet& ioApicSet, IApic *apic, uint8_t irq);
}
