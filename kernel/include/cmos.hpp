#pragma once

#include <cstdint>

namespace km {
    void DisableNmi();
    void EnableNmi();

    struct DateTime {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    };

    void InitCmos(uint8_t century);

    DateTime ReadRtc();
}
