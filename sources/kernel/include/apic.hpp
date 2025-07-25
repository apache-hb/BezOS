#pragma once

#include "acpi/acpi.hpp"
#include "arch/paging.hpp"
#include "memory/detail/tlsf.hpp"
#include "util/combine.hpp"
#include "util/absl.hpp"

namespace km {
    class SystemMemory;
    class AddressSpace;

    namespace apic {
        enum class IcrDeliver : uint32_t {
            eSingle = 0b00,
            eSelf = 0b01,
            eAll = 0b10,
            eOther = 0b11,
        };

        enum class IcrMode : uint32_t {
            eFixed = 0b000,
            eLowest = 0b001,
            eSmi = 0b010,
            eNmi = 0b100,
            eInit = 0b101,
            eStartup = 0b110,
        };

        enum class Ivt {
            eTimer = 0x32,
            eThermal = 0x33,
            ePerformance = 0x34,
            eLvt0 = 0x35,
            eLvt1 = 0x36,
            eError = 0x37,
        };

        enum class Polarity : uint32_t {
            eActiveHigh = 0,
            eActiveLow = 1,
        };

        enum class Trigger : uint32_t {
            eEdge = 0,
            eLevel = 1,
        };

        enum class DestinationMode : uint32_t {
            ePhysical = 0,
            eLogical = 1,
        };

        enum class Level : uint32_t {
            eDeAssert = 0,
            eAssert = 1,
        };

        enum class TimerMode {
            eOneShot = 0b00,
            ePeriodic = 0b01,
            eDeadline = 0b10,
            eNone = 0b11
        };

        struct IvtConfig {
            uint8_t vector;
            Polarity polarity;
            Trigger trigger;
            bool enabled;
            TimerMode timer = TimerMode::eNone;
        };

        enum class Type {
            eLocalApic,
            eX2Apic,
        };

        struct IpiAlert {
            uint8_t vector;
            IcrMode mode = IcrMode::eFixed;
            DestinationMode dst = DestinationMode::ePhysical;
            Trigger trigger = Trigger::eEdge;
            Level level = Level::eAssert;

            static constexpr IpiAlert sipi(km::PhysicalAddressEx address) {
                return IpiAlert {
                    .vector = uint8_t(address.address / x64::kPageSize),
                    .mode = IcrMode::eStartup,
                    .dst = DestinationMode::ePhysical,
                    .trigger = Trigger::eEdge,
                    .level = Level::eAssert,
                };
            }

            static constexpr IpiAlert init() {
                return IpiAlert {
                    .vector = 0,
                    .mode = IcrMode::eInit,
                    .dst = DestinationMode::ePhysical,
                    .trigger = Trigger::eEdge,
                    .level = Level::eAssert,
                };
            }
        };

        enum class TimerDivide {
            e1 = 0b111,
            e2 = 0b000,
            e4 = 0b001,
            e8 = 0b010,
            e16 = 0b011,
            e32 = 0b100,
            e64 = 0b101,
            e128 = 0b110,
        };

        struct ErrorState {
            bool egressChecksum:1;
            bool ingressChecksum:1;
            bool egressAccept:1;
            bool ingressAccept:1;
            bool priorityIpi:1;
            bool egressVector:1;
            bool ingressVector:1;
            bool illegalRegister:1;
        };

        // apic register offsets

        static constexpr uint16_t kApicVersion = 0x3;
        static constexpr uint16_t kEndOfInt = 0xb;
        static constexpr uint16_t kSpuriousInt = 0xf;
        static constexpr uint16_t kErrorStatus = 0x28;
        static constexpr uint16_t kTimerLvt = 0x32;
        static constexpr uint16_t kTaskPriority = 0x8;

        static constexpr uint16_t kInitialCount = 0x38;
        static constexpr uint16_t kCurrentCount = 0x39;

        static constexpr uint16_t kDivide = 0x3e;

        static constexpr uint16_t kTscDeadline = 0x6e;

        static constexpr uint16_t kIcr0 = 0x30;
        static constexpr uint16_t kIcr1 = 0x31;

        /// Intel SDM Vol 3A Table 12-1. Local APIC Register Address Map
        struct ApicMmioRegisters {
            alignas(0x10) uint32_t reserved0;
            alignas(0x10) uint32_t reserved1;
            alignas(0x10) uint32_t id;
            alignas(0x10) uint32_t version;
            alignas(0x10) uint32_t reserved2;
            alignas(0x10) uint32_t reserved3;
            alignas(0x10) uint32_t reserved4;
            alignas(0x10) uint32_t reserved5;
            alignas(0x10) uint32_t tpr;
            alignas(0x10) uint32_t apr;
            alignas(0x10) uint32_t ppr;
            alignas(0x10) uint32_t eoi;
            alignas(0x10) uint32_t rrd;
            alignas(0x10) uint32_t dst;
            alignas(0x10) uint32_t dfr;
            alignas(0x10) uint32_t spurious;

        };
    }

    void Disable8259Pic();
    void EnableX2Apic();
    bool HasX2ApicSupport();
    bool IsX2ApicEnabled();

    class IApic {
        void maskTaskPriority();
        void enableSpuriousInt();

        virtual void writeIcr(uint32_t dst, uint32_t cmd) = 0;

        virtual uint64_t read(uint16_t offset) const = 0;
        virtual void write(uint16_t offset, uint64_t value) noexcept [[clang::reentrant]] = 0;

    public:
        virtual ~IApic() = default;

        virtual uint32_t id() const = 0;
        virtual apic::Type type() const = 0;

        virtual void selfIpi(uint8_t vector) = 0;

        uint32_t version() const;

        void sendIpi(apic::IcrDeliver deliver, uint8_t vector);
        void sendIpi(uint32_t dst, apic::IpiAlert alert);
        void sendIpi(apic::IcrDeliver deliver, apic::IpiAlert alert);

        void cfgIvtTimer(apic::IvtConfig config) { configure(apic::Ivt::eTimer, config); }
        void cfgIvtThermal(apic::IvtConfig config) { configure(apic::Ivt::eThermal, config); }
        void cfgIvtPerformance(apic::IvtConfig config) { configure(apic::Ivt::ePerformance, config); }
        void cfgIvtLvt0(apic::IvtConfig config) { configure(apic::Ivt::eLvt0, config); }
        void cfgIvtLvt1(apic::IvtConfig config) { configure(apic::Ivt::eLvt1, config); }
        void cfgIvtError(apic::IvtConfig config) { configure(apic::Ivt::eError, config); }

        void maskIvt(apic::Ivt ivt);

        virtual bool pendingIpi() = 0;

        apic::ErrorState status();

        void configure(apic::Ivt ivt, apic::IvtConfig config);

        void setTimerDivisor(apic::TimerDivide timer);

        void setInitialCount(uint64_t count);
        uint64_t getCurrentCount();

        void eoi() noexcept [[clang::reentrant]];

        void enable();

        void setSpuriousVector(uint8_t vector);
    };

    class X2Apic final : public IApic {
        uint64_t read(uint16_t offset) const override;
        void write(uint16_t offset, uint64_t value) noexcept [[clang::reentrant]] override;

        void writeIcr(uint32_t icr0, uint32_t icr1) override;

    public:
        constexpr X2Apic() = default;

        static X2Apic get() { return X2Apic(); }

        uint32_t id() const override;
        apic::Type type() const override { return apic::Type::eX2Apic; }

        void selfIpi(uint8_t vector) override;
        bool pendingIpi() override { return false; }
    };

    class LocalApic final : public IApic {
        static constexpr uint16_t kApicId = 0x20;

        static constexpr uint32_t kIcr1 = 0x310;
        static constexpr uint32_t kIcr0 = 0x300;

        TlsfAllocation mAllocation;
        sm::VirtualAddress mAddress{nullptr};

        volatile uint32_t& reg(uint16_t offset) const;

        uint64_t read(uint16_t offset) const override;
        void write(uint16_t offset, uint64_t value) noexcept [[clang::reentrant]] override;

        void writeIcr(uint32_t icr0, uint32_t icr1) override;

    public:
        constexpr LocalApic() = default;

        constexpr LocalApic(TlsfAllocation allocation, sm::VirtualAddress base) noexcept
            : mAllocation(allocation)
            , mAddress(base)
        { }

        uint32_t id() const override;
        apic::Type type() const override { return apic::Type::eLocalApic; }

        void selfIpi(uint8_t vector) override;
        bool pendingIpi() override;

        sm::VirtualAddress baseAddress() const { return mAddress; }
    };

    using Apic = sm::Combine<IApic, LocalApic, X2Apic>;

    namespace detail {
        struct alignas(0x10) IoApicRegister {
            volatile uint32_t reg;

            operator volatile uint32_t&(this auto&& self) { return self.reg; }
            IoApicRegister& operator=(uint32_t value) {
                reg = value;
                return *this;
            }
        };

        struct IoApicMmio {
            IoApicRegister ioregsel;
            IoApicRegister iowin;
        };
    }

    class IoApic {
        km::TlsfAllocation mAllocation;
        sm::VirtualAddressOf<detail::IoApicMmio> mMmio{nullptr};
        uint32_t mIsrBase;
        uint8_t mId;

        void select(uint32_t field);
        uint32_t read(uint32_t reg);
        void write(uint32_t field, uint32_t value);

    public:
        IoApic() = default;
        IoApic(const acpi::MadtEntry *entry, km::AddressSpace& memory);
        IoApic(acpi::MadtEntry::IoApic entry, km::AddressSpace& memory);

        uint8_t id() const { return mId; }
        uint32_t isrBase() const { return mIsrBase; }
        const void *address() const { return mMmio; }

        uint16_t inputCount();
        uint8_t version();
        uint8_t arbitrationId();

        /// @brief Update the redirection table entry for the given ISR
        ///
        /// @param config The configuration for the ISR
        /// @param redirect The redirection table entry to update
        /// @param target The target APIC to send the interrupt to
        void setRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target);

        void clearRedirect(uint32_t redirect);

        bool present() const { return !mMmio.isNull(); }
    };

    class IoApicSet {
        const acpi::Madt *mMadt;
        sm::FixedArray<IoApic> mIoApics;

    public:
        IoApicSet(const acpi::Madt *madt, km::AddressSpace& memory);

        uint32_t count() const { return mIoApics.size(); }
        auto&& operator[](this auto&& self, uint32_t index) { return self.mIoApics[index]; }

        /// @brief Update the redirection table entry for the given ISR
        ///
        /// @param config The configuration for the ISR
        /// @param redirect The redirection table entry to update
        /// @param target The target APIC to send the interrupt to
        void setRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target);

        /// @brief Update the redirection table entry for the given ISR
        ///
        /// This function will attempt to use the MADT to find the
        /// correct redirection table entry for the given ISR.
        ///
        /// @param config The configuration for the ISR
        /// @param redirect The redirection table entry to update
        /// @param target The target APIC to send the interrupt to
        void setLegacyRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target);

        void clearRedirect(uint32_t redirect);

        void clearLegacyRedirect(uint32_t redirect);
    };

    Apic InitBspApic(km::AddressSpace& memory, bool useX2Apic);
    Apic InitApApic(km::AddressSpace& memory, const km::IApic *bsp);
}

template<>
struct km::Format<km::apic::ErrorState> {
    static void format(km::IOutStream& out, km::apic::ErrorState esr);
};
