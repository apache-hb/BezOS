#pragma once

#include "apic.hpp"
#include "isr.hpp"

#include <cstdint>

namespace km {
    class Pit {
    public:
        static constexpr unsigned kFrequencyHz = 1'193182;

        void setFrequency(unsigned frequency);

        uint16_t getCount();
        void setCount(uint16_t count);
    };

    void InitPit(unsigned frequency, const acpi::Madt *madt, IoApic& ioApic, IApic *apic, IsrAllocator& isrAllocator);
}
