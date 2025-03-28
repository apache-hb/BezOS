#pragma once

#include "acpi/header.hpp"

namespace acpi {
    class IGenericAddress {
    public:
        virtual ~IGenericAddress() = default;
        virtual GenericAddress address() const = 0;

        virtual uint64_t load() = 0;
        virtual void store(uint64_t value) = 0;
    };
}
