#pragma once

#include "acpi/header.hpp"
#include "util/signature.hpp"

namespace acpi {
    struct [[gnu::packed]] Hpet {
        static constexpr TableSignature kSignature = util::Signature("HPET");

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
