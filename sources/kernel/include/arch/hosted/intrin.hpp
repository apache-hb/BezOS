#pragma once

#include "arch/generic/intrin.hpp"

namespace arch {
    class IHostedIntrin {
    public:
        virtual ~IHostedIntrin() = default;

        virtual void nop() noexcept { }
        virtual void halt() noexcept { }
        virtual void cli() noexcept { }
        virtual void sti() noexcept { }
        virtual void invlpg(uintptr_t) noexcept { }
        virtual uint64_t rdmsr(uint32_t) noexcept { return 0; }
        virtual void wrmsr(uint32_t, uint64_t) noexcept { }

        static IHostedIntrin *GetDefault() noexcept {
            static IHostedIntrin sInstance;
            return &sInstance;
        }
    };

    struct HostedIntrin : GenericIntrin {
        static IHostedIntrin *gImpl;

        static void nop() noexcept {
            gImpl->nop();
        }

        static void halt() noexcept {
            gImpl->halt();
        }

        static void cli() noexcept {
            gImpl->cli();
        }

        static void sti() noexcept {
            gImpl->sti();
        }

        static void invlpg(uintptr_t address) noexcept {
            gImpl->invlpg(address);
        }

        static uint64_t rdmsr(uint32_t msr) noexcept {
            return gImpl->rdmsr(msr);
        }

        static void wrmsr(uint32_t msr, uint64_t value) noexcept {
            gImpl->wrmsr(msr, value);
        }
    };

    using Intrin = HostedIntrin;
}
