#pragma once

#include "acpi/header.hpp"

namespace acpi {
    struct [[gnu::packed]] Hpet {
        static constexpr std::array<char, 4> kSignature = { 'H', 'P', 'E', 'T' };

        RsdtHeader header;
        uint32_t evtTimerBlockId;
        GenericAddress baseAddress;
        uint8_t hpetNumber;
        uint16_t clockTick;
        uint8_t pageProtection;

        uint8_t comparatorCount() const {
            return (evtTimerBlockId >> 8) & 0b11111;
        }

        uint16_t vendorId() const {
            return evtTimerBlockId >> 16;
        }
    };

    static_assert(sizeof(Hpet) == 56);
}
