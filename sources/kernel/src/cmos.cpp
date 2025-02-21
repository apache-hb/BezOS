#include "cmos.hpp"

#include "delay.hpp"
#include "isr/isr.hpp"

static constexpr uint16_t kCmosSelect = 0x70;
static constexpr uint16_t kCmosData = 0x71;

static constexpr uint8_t kDisableNmi = (1 << 7);

static uint8_t gCenturyRegister = 0;

void km::DisableNmi() {
    uint8_t nmi = KmReadByte(kCmosSelect);
    KmWriteByte(kCmosData, nmi & ~kDisableNmi);
}

void km::EnableNmi() {
    uint8_t nmi = KmReadByte(kCmosSelect);
    KmWriteByte(kCmosData, nmi | kDisableNmi);
}

static uint8_t ReadCmosRegister(uint8_t reg) {
    KmWriteByte(kCmosSelect, reg);
    return KmReadByte(kCmosData);
}

void km::InitCmos(uint8_t century) {
    gCenturyRegister = century;
}

static uint8_t ConvertFromBcd(uint8_t value) {
    return ((value / 16) * 10) + (value & 0xF);
}

static void DateComponentOverflow(uint8_t& value, uint8_t& next, uint8_t max) {
    if (value >= max) {
        value -= max;
        next += 1;
    }
}

km::detail::CmosRegisters km::detail::ReadCmosRegisters(uint8_t centuryRegister) {
    km::NmiGuard guard;

    return km::detail::CmosRegisters {
        .regB = ReadCmosRegister(0x0B),
        .second = ReadCmosRegister(0x00),
        .minute = ReadCmosRegister(0x02),
        .hour = ReadCmosRegister(0x04),
        .day = ReadCmosRegister(0x07),
        .month = ReadCmosRegister(0x08),
        .year = ReadCmosRegister(0x09),
        .century = (centuryRegister != 0)
            ? ReadCmosRegister(centuryRegister)
            : uint8_t(0),
    };
}

km::DateTime km::detail::ConvertCmosToDate(detail::CmosRegisters registers) {
    auto [regB, second, minute, hour, day, month, year, century] = registers;

    bool is12Hour = regB & k12HourClock;
    bool isBcdFormat = !(regB & kDecimalMode);
    bool isPm = (hour & kPmHour);

    if (isBcdFormat) {
        second = ConvertFromBcd(second);
        minute = ConvertFromBcd(minute);
        hour = ConvertFromBcd(hour & ~kPmHour);
        day = ConvertFromBcd(day);
        month = ConvertFromBcd(month);
        year = ConvertFromBcd(year);
    }

    if (is12Hour && isPm) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    DateComponentOverflow(second, minute, 60);
    DateComponentOverflow(minute, hour, 60);
    DateComponentOverflow(hour, day, 24);

    if (century != 0) {
        if (isBcdFormat)
            century = ConvertFromBcd(century);

        year += century * 100;
    } else {
        year += kDefaultYear;
    }

    return DateTime {
        .second = second,
        .minute = minute,
        .hour = hour,
        .day = day,
        .month = month,
        .year = year,
    };
}

km::DateTime km::ReadCmosClock() {
    auto regs = detail::ReadCmosRegisters(gCenturyRegister);
    return detail::ConvertCmosToDate(regs);
}
