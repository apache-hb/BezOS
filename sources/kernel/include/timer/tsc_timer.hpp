#pragma once

#include <bezos/status.h>

#include "arch/msr.hpp"

#include "timer/tick_source.hpp"

static constexpr x64::RoModelRegister<0x10> IA32_TSC;

namespace km {
    class InvariantTsc;

    OsStatus trainInvariantTsc(ITickSource *refclk, InvariantTsc *tsc);

    class InvariantTsc final : public ITickSource {
        hertz mFrequency;

        InvariantTsc(hertz frequency)
            : mFrequency(frequency)
        { }

    public:
        constexpr InvariantTsc() = default;

        TickSourceType type() const override { return TickSourceType::TSC; }
        hertz refclk() const override { return mFrequency; }
        hertz frequency() const override { return mFrequency; }
        uint64_t ticks() const override { return __builtin_ia32_rdtsc(); }

        friend OsStatus km::trainInvariantTsc(ITickSource *refclk, InvariantTsc *tsc);
    };
}
