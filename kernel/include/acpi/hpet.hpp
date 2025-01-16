#pragma once

#include "acpi/header.hpp"

namespace acpi {
    struct [[gnu::packed]] Hpet {
        RsdtHeader header;
        uint32_t evtTimerBlockId;
        GenericAddress baseAddress;
        uint8_t hpetNumber;
        uint16_t clockTick;
        uint8_t pageProtection;
    };

    static_assert(sizeof(Hpet) == 56);
}
