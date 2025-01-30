#pragma once

#include "acpi/hpet.hpp"
#include "apic.hpp"
#include "isr.hpp"

#include <cstdint>

// workaround for clang bug, nodiscard in a requires clause triggers a warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

#include <mp-units/systems/si.h>

#pragma clang diagnostic pop

namespace mp = mp_units;
namespace si = mp_units::si;

namespace km {
    using hertz = mp::quantity<si::hertz, int>;

    namespace pit {
        enum class Type {
            PIT8254,
            HPET,
        };
    }

    class ITickSource {
    public:
        virtual ~ITickSource() = default;

        void operator delete(ITickSource*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual pit::Type type() const = 0;

        virtual hertz refclk() const = 0;

        virtual uint64_t ticks() const = 0;
    };

    class IntervalTimer final : public ITickSource {
    public:
        static constexpr hertz kFrequencyHz = 1'193182 * si::hertz;

        void setFrequency(hertz frequency);

        uint16_t getCount() const;
        void setCount(uint16_t count);

        pit::Type type() const override { return pit::Type::PIT8254; }

        hertz refclk() const override {
            return kFrequencyHz;
        }

        uint64_t ticks() const override {
            return getCount();
        }
    };

    class HighPrecisionTimer final : public ITickSource {
        acpi::Hpet mTable;

    public:
        pit::Type type() const override { return pit::Type::HPET; }

        HighPrecisionTimer(const acpi::Hpet *hpet, SystemMemory& memory);

        hertz refclk() const override;

        uint64_t ticks() const override;
    };

    using TickSource = sm::Combine<ITickSource, IntervalTimer, HighPrecisionTimer>;

    void InitPit(hertz frequency, const acpi::Madt *madt, IoApic& ioApic, IApic *apic, uint8_t irq, IsrCallback handler);
}
