#pragma once

#include "memory/allocator.hpp"
#include "memory/layout.hpp"
namespace km {
    namespace apic {
        enum IcrDeliver {
            eSelf = 0b01,
            eAll = 0b10,
            eOther = 0b11,
        };
    }

    class LocalAPIC {
        static constexpr uint16_t kSpuriousInt = 0xF0;
        static constexpr uint16_t kEndOfInt = 0xB0;
        static constexpr uint16_t kTimerLvt = 0x320;
        static constexpr uint16_t kApicId = 0x20;
        static constexpr uint16_t kApicVersion = 0x30;

        static constexpr uint16_t kTimerIvt = 0x320;
        static constexpr uint16_t kThermalIvt = 0x330;
        static constexpr uint16_t kPerformanceIvt = 0x340;
        static constexpr uint16_t kLint0Ivt = 0x350;
        static constexpr uint16_t kLint1Ivt = 0x360;
        static constexpr uint16_t kErrorIvt = 0x370;

        static constexpr uint32_t kIcrHigh = 0x310;
        static constexpr uint32_t kIcrLow = 0x300;

        static constexpr uint32_t kApicEnable = (1 << 8);

        km::VirtualAddress mBaseAddress;

        volatile uint32_t& reg(uint16_t offset) const noexcept {
            return *reinterpret_cast<volatile uint32_t*>(mBaseAddress.address + offset);
        }

    public:
        LocalAPIC(km::VirtualAddress base) noexcept
            : mBaseAddress(base)
        { }

        uint32_t id(void) const noexcept {
            return reg(kApicId);
        }

        uint32_t version(void) const noexcept {
            return reg(kApicVersion) & 0xFF;
        }

        uint32_t spuriousInt(void) const noexcept {
            return reg(kSpuriousInt);
        }

        void setSpuriousInt(uint32_t value) noexcept {
            reg(kSpuriousInt) = value;
        }

        uint32_t timerLvt(void) const noexcept {
            return reg(kTimerIvt);
        }

        void setTimerLvt(uint32_t value) noexcept {
            reg(kTimerIvt) = value;
        }

        void sendIpi(uint32_t dst, uint8_t vector) noexcept {
            reg(kIcrHigh) = dst << 24;
            reg(kIcrLow) = vector;
        }

        void sendIpi(apic::IcrDeliver deliver, uint8_t vector) noexcept {
            reg(kIcrLow) = deliver << 18 | uint16_t(vector);
        }

        void clearEndOfInterrupt(void) noexcept {
            reg(kEndOfInt) = 0;
        }

        void enable(void) noexcept {
            setSpuriousInt(spuriousInt() | kApicEnable);
        }

        void setSpuriousVector(uint8_t vector) noexcept {
            setSpuriousInt((spuriousInt() & 0xFF) | vector);
        }
    };
}

void KmDisablePIC(void);
km::LocalAPIC KmGetLocalAPIC(km::VirtualAllocator& vmm, const km::PageManager& pm);
