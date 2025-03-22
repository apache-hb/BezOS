#pragma once

#include "units.hpp"

#include <chrono>
#include <stdint.h>

namespace km {
    using hertz = mp::quantity<si::hertz, uint64_t>;

    enum class TickSourceType {
        PIT8254,
        HPET,
        APIC,
        TSC,
    };

    class ITickSource {
    public:
        virtual ~ITickSource() = default;

        virtual TickSourceType type() const = 0;
        virtual hertz refclk() const = 0;
        virtual hertz frequency() const = 0;
        virtual uint64_t ticks() const = 0;
    };

    void BusySleep(ITickSource *timer, std::chrono::microseconds duration);
}
