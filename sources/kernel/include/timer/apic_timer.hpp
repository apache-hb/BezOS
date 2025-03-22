#pragma once

#include <bezos/status.h>

#include "timer/tick_source.hpp"

namespace km {
    class IApic;
    class ApicTimer;

    OsStatus TrainApicTimer(IApic *apic, ITickSource *refclk, ApicTimer *timer);

    class ApicTimer final : public ITickSource {
        hertz mFrequency = hertz::zero();
        IApic *mApic = nullptr;

        ApicTimer(hertz frequency, IApic *apic)
            : mFrequency(frequency)
            , mApic(apic)
        { }

    public:
        constexpr ApicTimer() = default;

        TickSourceType type() const override { return TickSourceType::APIC; }
        hertz refclk() const override { return mFrequency; }
        hertz frequency() const override { return mFrequency; }
        uint64_t ticks() const override;

        friend OsStatus km::TrainApicTimer(IApic *apic, ITickSource *refclk, ApicTimer *timer);
    };
}
