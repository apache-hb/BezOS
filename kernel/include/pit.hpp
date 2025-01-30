#pragma once

#include "acpi/hpet.hpp"
#include "apic.hpp"
#include "isr.hpp"
#include "pci.hpp"

#include <cstdint>

// workaround for clang bug, nodiscard in a requires clause triggers a warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

#include <mp-units/systems/si.h>

#pragma clang diagnostic pop

namespace mp = mp_units;
namespace si = mp_units::si;

namespace km {
    using hertz = mp::quantity<si::hertz, int64_t>;

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

    struct [[gnu::packed]] HpetComparator {
        volatile uint64_t config;
        volatile uint64_t comparator;
        volatile uint64_t irq;
        uint8_t reserved[8];
    };

    struct [[gnu::packed]] HpetRegisters {
        volatile const uint64_t id;
        uint8_t reserved0[8];
        volatile uint64_t config;
        uint8_t reserved1[8];
        volatile uint64_t irqStatus;
        uint8_t reserved2[200];
        volatile uint64_t counter;
        uint8_t reserved3[8];

        /// @warning Not all comparators may be available.
        ///          The full 32 are specified here to ensure they're all mapped.
        HpetComparator comparators[32];
    };

    static_assert(offsetof(HpetRegisters, id) == 0x00);
    static_assert(offsetof(HpetRegisters, config) == 0x10);
    static_assert(offsetof(HpetRegisters, irqStatus) == 0x20);
    static_assert(offsetof(HpetRegisters, counter) == 0xF0);
    static_assert(offsetof(HpetRegisters, comparators) == 0x100);

    enum class HpetWidth {
        DWORD,
        QWORD,
    };

    class HighPrecisionTimer final : public ITickSource {
        acpi::Hpet mTable;
        HpetRegisters *mMmioRegion;

        HighPrecisionTimer(const acpi::Hpet *hpet, SystemMemory& memory);

    public:
        pit::Type type() const override;
        uint16_t bestDivisor(hertz frequency) const override;
        hertz refclk() const override;
        uint64_t ticks() const override;
        void setDivisor(uint16_t divisor) override;

        pci::VendorId vendor() const;
        HpetWidth counterSize() const;
        uint8_t timerCount() const;
        uint8_t revision() const;

        void enable(bool enabled);

        bool isTimerActive(uint8_t timer) const;

        static std::optional<HighPrecisionTimer> find(const acpi::AcpiTables& acpiTables, SystemMemory& memory);
    };

    using TickSource = sm::Combine<ITickSource, IntervalTimer, HighPrecisionTimer>;

    void InitPit(hertz frequency, const acpi::Madt *madt, IoApic& ioApic, IApic *apic, uint8_t irq, IsrCallback handler);
}
