#pragma once

#include <cstdint>

namespace km {
    static constexpr uint16_t kDefaultYear = 2000;
    static constexpr uint8_t k12HourClock = (1 << 1);
    static constexpr uint8_t kDecimalMode = (1 << 2);
    static constexpr uint8_t kPmHour = (1 << 7);

    struct DateTime {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    };

    namespace detail {
        struct CmosRegisters {
            uint8_t regB;
        
            uint8_t second;
            uint8_t minute;
            uint8_t hour;
            uint8_t day;
            uint8_t month;
            uint16_t year;
            uint8_t century;
        };

        CmosRegisters ReadCmosRegisters(uint8_t centuryRegister);
        DateTime ConvertCmosToDate(CmosRegisters registers);
    }

    void DisableNmi();
    void EnableNmi();

    void InitCmos(uint8_t century);

    DateTime ReadRtc();
}
