#pragma once

#include "acpi/acpi.hpp"
#include "util/combine.hpp"

#include <utility>

namespace km {
    namespace apic {
        enum class IcrDeliver : uint32_t {
            eSelf = 0b01,
            eAll = 0b10,
            eOther = 0b11,
        };

        enum class Ivt {
            eTimer = 0x32,
            eThermal = 0x33,
            ePerformance = 0x34,
            eLvt0 = 0x35,
            eLvt1 = 0x36,
            eError = 0x37,
        };

        enum class Polarity {
            eActiveHigh = 0,
            eActiveLow = 1,
        };

        enum class Trigger {
            eEdge = 0,
            eLevel = 1,
        };

        struct IvtConfig {
            uint8_t vector;
            Polarity polarity;
            Trigger trigger;
            bool enabled;
        };

        enum class Type {
            eLocalApic,
            eX2Apic,
        };

        // apic register offsets

        static constexpr uint16_t kApicVersion = 0x3;
        static constexpr uint16_t kEndOfInt = 0xB;
        static constexpr uint16_t kSpuriousInt = 0xF;
        static constexpr uint16_t kTimerLvt = 0x32;
        static constexpr uint16_t kTaskPriority = 0x8;

        static constexpr uint16_t kIcr1 = 0x31;
        static constexpr uint16_t kIcr0 = 0x30;
    }

    void EnableX2Apic();
    bool HasX2ApicSupport();
    bool IsX2ApicEnabled();

    class IApic {
        void maskTaskPriority();
        void enableSpuriousInt();
    public:
        void operator delete(IApic*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual ~IApic() = default;

        virtual uint32_t read(uint16_t offset) const = 0;
        virtual void write(uint16_t offset, uint32_t value) = 0;

        virtual uint32_t id() const = 0;
        virtual apic::Type type() const = 0;

        virtual void sendIpi(uint32_t dst, uint32_t vector) = 0;

        uint32_t version() const;

        void eoi();

        void sendIpi(apic::IcrDeliver deliver, uint8_t vector);

        void cfgIvtTimer(apic::IvtConfig config) { configure(apic::Ivt::eTimer, config); }
        void cfgIvtThermal(apic::IvtConfig config) { configure(apic::Ivt::eThermal, config); }
        void cfgIvtPerformance(apic::IvtConfig config) { configure(apic::Ivt::ePerformance, config); }
        void cfgIvtLvt0(apic::IvtConfig config) { configure(apic::Ivt::eLvt0, config); }
        void cfgIvtLvt1(apic::IvtConfig config) { configure(apic::Ivt::eLvt1, config); }
        void cfgIvtError(apic::IvtConfig config) { configure(apic::Ivt::eError, config); }

        void configure(apic::Ivt ivt, apic::IvtConfig config);

        void enable();

        void setSpuriousVector(uint8_t vector);
    };

    class X2Apic final : public IApic {
        uint32_t read(uint16_t offset) const override;
        void write(uint16_t offset, uint32_t value) override;

    public:
        constexpr X2Apic() = default;

        static X2Apic get() { return X2Apic(); }

        uint32_t id() const override;
        apic::Type type() const override { return apic::Type::eX2Apic; }

        void sendIpi(uint32_t dst, uint32_t vector) override;
    };

    class LocalApic final : public IApic {
        static constexpr uint16_t kApicId = 0x20;

        static constexpr uint32_t kIcr1 = 0x310;
        static constexpr uint32_t kIcr0 = 0x300;

        void *mBaseAddress = nullptr;

        volatile uint32_t& reg(uint16_t offset) const;

        uint32_t read(uint16_t offset) const override;
        void write(uint16_t offset, uint32_t value) override;

    public:
        constexpr LocalApic() = default;

        constexpr LocalApic(void *base)
            : mBaseAddress(base)
        { }

        uint32_t id() const override;
        apic::Type type() const override { return apic::Type::eLocalApic; }

        void sendIpi(uint32_t dst, uint32_t vector) override;

        void *baseAddress(void) const { return mBaseAddress; }
    };

    using Apic = sm::Combine<IApic, LocalApic, X2Apic>;

    class IoApic {
        uint8_t *mAddress = nullptr;
        uint32_t mIsrBase;
        uint8_t mId;

        volatile uint32_t& reg(uint32_t offset);

        void select(uint32_t field);
        uint32_t read(uint32_t reg);
        void write(uint32_t field, uint32_t value);

    public:
        IoApic() = default;
        IoApic(const acpi::MadtEntry *entry, km::SystemMemory& memory);

        uint8_t id() const { return mId; }
        uint32_t isrBase() const { return mIsrBase; }

        uint16_t inputCount();
        uint8_t version();

        /// @brief Update the redirection table entry for the given ISR
        ///
        /// @param config The configuration for the ISR
        /// @param redirect The redirection table entry to update
        /// @param target The target APIC to send the interrupt to
        void setRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target);

        void setLegacyRedirect(apic::IvtConfig config, uint32_t redirect, const acpi::Madt *madt, const IApic *target);

        bool present() const { return mAddress != nullptr; }
    };

    Apic InitBspApic(km::SystemMemory& memory, bool useX2Apic);
    Apic InitApApic(km::SystemMemory& memory, const km::IApic *bsp);
}
