#pragma once

#include "acpi/header.hpp"

#include <stddef.h>

namespace acpi {
    struct [[gnu::packed]] McfgAllocation {
        uint64_t address;
        uint16_t segment;
        uint8_t startBusNumber;
        uint8_t endBustNumber;

        uint8_t reserved0[4];
    };

    static_assert(sizeof(McfgAllocation) == 16);

    struct [[gnu::packed]] Mcfg {
        RsdtHeader header; // signature must be "MCFG"
        uint8_t reserved0[8];

        McfgAllocation allocations[];

        size_t allocationCount() const {
            return (header.length - sizeof(Mcfg)) / sizeof(McfgAllocation);
        }
    };
}
