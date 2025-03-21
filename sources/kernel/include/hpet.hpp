#pragma once

#include "acpi/hpet.hpp"
#include "pci/pci.hpp"

#include "pit.hpp"

namespace km {
    namespace hpet {
        using period = mp::quantity<si::femto<si::second>, uint32_t>;

        static constexpr inline period kMaxPeriod = 100000000 * si::femto<si::second>;
        static constexpr inline period kMinPeriod = 100000 * si::femto<si::second>;

        enum class Width : uint_least8_t {
            DWORD = 1,
            QWORD = 0,
        };

        enum class Mode {
            eOneShot = 0,
            ePeriodic = 1,
        };

        class HpetId {
            uint64_t mValue;
        public:
            HpetId() : mValue(0) { }
            HpetId(uint64_t value) : mValue(value) { }

            period refperiod() const;
            pci::VendorId vendorId() const;
            bool legacyRtCapable() const;
            Width counterSize() const;
            uint8_t timerCount() const;
            uint8_t revision() const;
        };

        struct ComparatorConfig {
            uint8_t ioApicRoute;
            Width mode;
            bool enable;
            apic::Trigger trigger;
            bool periodic;

            uint64_t period;
        };

        class [[gnu::packed]] Comparator {
            /// @note Tn_CAP
            volatile uint64_t mConfig;
            /// @note Tn_COUNTER
            volatile uint64_t mCounter;

            /// @note Tn_FSB_INT_ROUTE
            [[maybe_unused]]
            volatile uint64_t mFsbRoute;

            [[maybe_unused]]
            uint8_t mReserved[8];

        public:
            /// @note Tn_INT_ROUTE_CAP
            uint32_t routeMask() const;

            /// @note Tn_FSB_INT_DEL_CAP
            bool fsbIntDelivery() const;

            /// @note Tn_SIZE_CAP
            Width width() const;

            /// @note Tn_PER_INT_CAP
            bool periodicSupport() const;

            uint64_t counter() const;

            ComparatorConfig config() const;

            void configure(ComparatorConfig config);
        };

        struct [[gnu::packed]] MmioRegisters {
            volatile const uint64_t id;
            uint8_t reserved0[8];
            volatile uint64_t config;
            uint8_t reserved1[8];
            volatile uint64_t irqStatus;
            uint8_t reserved2[200];
            volatile uint64_t masterCounter;
            uint8_t reserved3[8];

            /// @warning Not all comparators may be available.
            ///          The full 32 are specified here to ensure they're all mapped.
            Comparator comparators[32];
        };

        static_assert(std::is_standard_layout_v<MmioRegisters>);
        static_assert(offsetof(MmioRegisters, id) == 0x00);
        static_assert(offsetof(MmioRegisters, config) == 0x10);
        static_assert(offsetof(MmioRegisters, irqStatus) == 0x20);
        static_assert(offsetof(MmioRegisters, masterCounter) == 0xF0);
        static_assert(offsetof(MmioRegisters, comparators) == 0x100);
    }

    OsStatus InitHpet(const acpi::AcpiTables& rsdt, SystemMemory& memory, HighPrecisionTimer *timer);

    class HighPrecisionTimer final {
        acpi::Hpet mTable;
        hpet::MmioRegisters *mMmioRegion;
        hpet::HpetId mId;

        HighPrecisionTimer(const acpi::Hpet *hpet, hpet::MmioRegisters *mmio);

    public:
        HighPrecisionTimer()
            : mMmioRegion(nullptr)
        { }

        pit::Type type() const;
        hertz refclk() const;
        uint64_t ticks() const;

        hpet::HpetId id() const { return hpet::HpetId { mMmioRegion->id }; }

        pci::VendorId vendor() const;
        hpet::Width counterSize() const;
        uint8_t timerCount() const;
        uint8_t revision() const;
        uint8_t hpetNumber() const { return mTable.hpetNumber; }

        void enable(bool enabled);

        bool isTimerActive(uint8_t timer) const;

        std::span<hpet::Comparator> comparators();
        std::span<const hpet::Comparator> comparators() const;

        friend OsStatus km::InitHpet(const acpi::AcpiTables& rsdt, SystemMemory& memory, HighPrecisionTimer *timer);
    };
}
