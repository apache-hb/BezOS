#pragma once

#include "arch/intrin.hpp"

#include <stdint.h>

namespace x64 {
    /// @brief Port I/O delay interface.
    /// Some platforms require a delay before a port write executes completely.
    /// There are also multiple methods of delaying port I/O depending on platform quirks
    /// which this interface is intended to encapsulate.
    class IPortDelay {
    public:
        virtual ~IPortDelay() = default;

        virtual void delay() = 0;
    };

    /// @brief Port delay for platforms that don't require delays.
    class NullPortDelay final : public IPortDelay {
        void delay() override { }
    };

    /// @brief Port delay that functions by writing 0 to port 0x80.
    /// This port is usually used to display status codes during POST.
    /// Writing to it is begnin on most platforms, and it forces a port flush.
    class PostCodePortDelay final : public IPortDelay {
        void delay() override {
            __outbyte(0x80, 0);
        }
    };
}
