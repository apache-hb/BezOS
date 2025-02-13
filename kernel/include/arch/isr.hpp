#pragma once

#include "util/digit.hpp"
#include <cstdint>

namespace x64 {
    struct [[gnu::packed]] IdtEntry {
        uint16_t address0;
        uint16_t selector;
        uint8_t ist;
        uint8_t flags;
        sm::uint48_t address1;
        uint32_t reserved;
    };

    static_assert(sizeof(IdtEntry) == 16);
}
