#pragma once

#include "acpi/header.hpp"

#include <stddef.h>

namespace acpi {
    struct [[gnu::packed]] McfgAllocation {
        uint64_t address;
        uint16_t segment;
        uint8_t startBusNumber;
        uint8_t endBusNumber;

        uint8_t reserved0[4];

        bool containsBus(uint8_t bus) const {
            return startBusNumber <= bus && endBusNumber >= bus;
        }
    };

    static_assert(sizeof(McfgAllocation) == 16);

    struct [[gnu::packed]] Mcfg {
        static constexpr TableSignature kSignature = { 'M', 'C', 'F', 'G' };

        RsdtHeader header; // signature must be "MCFG"
        uint8_t reserved0[8];

        McfgAllocation allocations[];

        size_t allocationCount() const {
            return (header.length - sizeof(Mcfg)) / sizeof(McfgAllocation);
        }

        std::span<const McfgAllocation> mcfgAllocations() const {
            return { allocations, allocationCount() };
        }

        const McfgAllocation *find(uint16_t segment, uint8_t bus) const {
            for (size_t i = 0; i < allocationCount(); i++) {
                const McfgAllocation *allocation = &allocations[i];

                if (allocation->segment == segment && allocation->containsBus(bus)) {
                    return allocation;
                }
            }

            return nullptr;
        }

    };
}
