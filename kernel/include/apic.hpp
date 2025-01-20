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
            eTimer = 0x320,
            eThermal = 0x330,
            ePerformance = 0x340,
            eLvt0 = 0x350,
            eLvt1 = 0x360,
            eError = 0x370,
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
    }

    void EnableX2Apic();
    bool HasX2ApicSupport();
    bool IsX2ApicEnabled();

    class IIntController {
    public:
        void operator delete(IIntController*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual ~IIntController() = default;

        virtual uint32_t id() const = 0;
        virtual uint32_t version() const = 0;
        virtual void eoi() = 0;

        virtual void sendIpi(uint32_t dst, uint32_t vector) = 0;

        void sendIpi(apic::IcrDeliver deliver, uint8_t vector);

        virtual void cfgIvtTimer(apic::IvtConfig) { }
        virtual void cfgIvtThermal(apic::IvtConfig) { }
        virtual void cfgIvtPerformance(apic::IvtConfig) { }
        virtual void cfgIvtLvt0(apic::IvtConfig) { }
        virtual void cfgIvtLvt1(apic::IvtConfig) { }
        virtual void cfgIvtError(apic::IvtConfig) { }

        virtual void enable() = 0;
        virtual void setSpuriousVector(uint8_t vector) = 0;
    };

    class X2Apic final : public IIntController {
    public:
        constexpr X2Apic() = default;

        static X2Apic get() { return X2Apic(); }

        uint32_t id() const override;

        uint32_t version() const override;

        void eoi() override;

        void sendIpi(uint32_t dst, uint32_t vector) override;

        void enable() override;
        void setSpuriousVector(uint8_t vector) override;
    };

    class LocalApic final : public IIntController {
        static constexpr uint16_t kApicId = 0x20;
        static constexpr uint16_t kApicVersion = 0x30;
        static constexpr uint16_t kEndOfInt = 0xB0;
        static constexpr uint16_t kSpuriousInt = 0xF0;
        static constexpr uint16_t kTimerLvt = 0x320;

        static constexpr uint32_t kIcr1 = 0x310;
        static constexpr uint32_t kIcr0 = 0x300;

        static constexpr uint32_t kIvtDisable = (1 << 16);

        void *mBaseAddress = nullptr;

        volatile uint32_t& reg(uint16_t offset) const;

        void setSpuriousInt(uint32_t value) {
            reg(kSpuriousInt) = value;
        }

        uint32_t spuriousInt() const {
            return reg(kSpuriousInt);
        }

    public:
        constexpr LocalApic() = default;

        constexpr LocalApic(void *base)
            : mBaseAddress(base)
        { }

        uint32_t id() const override;

        uint32_t version() const override;

        void sendIpi(uint32_t dst, uint32_t vector) override;

        void configure(apic::Ivt ivt, apic::IvtConfig config) {
            uint32_t entry
                = config.vector
                | (std::to_underlying(config.polarity) << 13)
                | (std::to_underlying(config.trigger) << 15)
                | ((config.enabled ? 0 : 1) << 16);

            reg(std::to_underlying(ivt)) = entry;
        }

        void eoi() override;

        void enable() override;

        void setSpuriousVector(uint8_t vector) override;

        void *baseAddress(void) const { return mBaseAddress; }
    };

    using IntController = sm::Combine<IIntController, LocalApic, X2Apic>;

    class IoApic {
        uint8_t *mAddress = nullptr;
        uint32_t mIsrBase;
        uint8_t mId;

        volatile uint32_t& reg(uint32_t offset);

        void select(uint32_t field);
        uint32_t read(uint32_t reg);

    public:
        IoApic() = default;
        IoApic(const acpi::MadtEntry *entry, km::SystemMemory& memory);

        uint8_t id() const { return mId; }
        uint32_t isrBase() const { return mIsrBase; }

        uint16_t inputCount();
        uint8_t version();

        bool present() const { return mAddress != nullptr; }
    };
}

km::IntController KmInitBspApic(km::SystemMemory& memory, bool useX2Apic);
km::IntController KmInitApApic(km::SystemMemory& memory, km::IIntController *bsp, bool useX2Apic);
