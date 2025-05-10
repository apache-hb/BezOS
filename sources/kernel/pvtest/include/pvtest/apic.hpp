#pragma once

#include <stdint.h>

namespace pv {
    class ApicState {
        uint8_t mApicId;

    public:
        ApicState(uint8_t apicId) : mApicId(apicId) {}

        uint8_t getApicId() const { return mApicId; }
    };
}
