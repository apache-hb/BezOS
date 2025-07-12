#pragma once

#include <cstdint>

#include "util/format.hpp"

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

    void InitCmos(uint8_t century) noexcept;

    DateTime ReadCmosClock();
}

template<>
struct km::Format<km::DateTime> {
    static void format(km::IOutStream& out, km::DateTime time) {
        out.format(time.year, "-", time.month, "-", time.day, "T", time.hour, ":", time.minute, ":", time.second, "Z");
    }
};
