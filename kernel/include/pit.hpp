#pragma once

#include <cstdint>

namespace km {
    class Pit {
    public:
        static constexpr unsigned kFrequencyHz = 1'193182;

        void setFrequency(unsigned frequency);

        uint16_t getCount();
        void setCount(uint16_t count);
    };

    void InitPit();
}
