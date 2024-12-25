#pragma once

#include "memory/allocator.hpp"
#include "memory/layout.hpp"
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

    class LocalAPIC {
        static constexpr uint16_t kSpuriousInt = 0xF0;
        static constexpr uint16_t kEndOfInt = 0xB0;
        static constexpr uint16_t kTimerLvt = 0x320;
        static constexpr uint16_t kApicId = 0x20;
        static constexpr uint16_t kApicVersion = 0x30;

        static constexpr uint32_t kIcrHigh = 0x310;
        static constexpr uint32_t kIcrLow = 0x300;

        static constexpr uint32_t kIvtDisable = (1 << 16);

        static constexpr uint32_t kApicEnable = (1 << 8);

        km::VirtualAddress mBaseAddress = nullptr;

        volatile uint32_t& reg(uint16_t offset) const {
            return *reinterpret_cast<volatile uint32_t*>(mBaseAddress.address + offset);
        }

    public:
        constexpr LocalAPIC() = default;

        LocalAPIC(km::VirtualAddress base)
            : mBaseAddress(base)
        { }

        uint32_t id(void) const {
            return reg(kApicId);
        }

        uint32_t version(void) const {
            return reg(kApicVersion) & 0xFF;
        }

        uint32_t spuriousInt(void) const {
            return reg(kSpuriousInt);
        }

        void setSpuriousInt(uint32_t value) {
            reg(kSpuriousInt) = value;
        }

        void sendIpi(uint32_t dst, uint8_t vector) {
            reg(kIcrHigh) = dst << 24;
            reg(kIcrLow) = vector;
        }

        void configure(apic::Ivt ivt, apic::IvtConfig config) {
            uint32_t entry
                = config.vector
                | (std::to_underlying(config.polarity) << 13)
                | (std::to_underlying(config.trigger) << 15)
                | ((config.enabled ? 0 : 1) << 16);

            reg(std::to_underlying(ivt)) = entry;
        }

        void sendIpi(apic::IcrDeliver deliver, uint8_t vector) {
            reg(kIcrHigh) = 0;
            reg(kIcrLow) = (std::to_underlying(deliver) << 18) | vector;
        }

        void clearEndOfInterrupt(void) {
            reg(kEndOfInt) = 0;
        }

        void enable(void) {
            setSpuriousInt(spuriousInt() | kApicEnable);
        }

        void setSpuriousVector(uint8_t vector) {
            setSpuriousInt((spuriousInt() & ~0xFF) | vector);
        }
    };

    class IoApic {

    public:
        uintptr_t mBaseAddress;
        IoApic();
    };
}

void KmDisablePIC(void);
km::LocalAPIC KmInitLocalAPIC(km::VirtualAllocator& vmm, const km::PageManager& pm);
