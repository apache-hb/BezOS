#include "cmos.hpp"

#include "delay.hpp"
#include "isr.hpp"
#include "log.hpp"

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
    KmDebugMessage("[CMOS] Century register: ", km::Hex(gCenturyRegister), "\n");
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

km::DateTime km::ReadRtc() {
    DisableNmi();
    DisableInterrupts();

    uint8_t regB = ReadCmosRegister(0x0B);

    uint8_t second = ReadCmosRegister(0x00);
    uint8_t minute = ReadCmosRegister(0x02);
    uint8_t hour = ReadCmosRegister(0x04);
    uint8_t day = ReadCmosRegister(0x07);
    uint8_t month = ReadCmosRegister(0x08);
    uint16_t year = ReadCmosRegister(0x09);
    uint8_t century = (gCenturyRegister != 0)
        ? ReadCmosRegister(gCenturyRegister)
        : 0;

    EnableInterrupts();
    EnableNmi();

    bool is24Hour = !(regB & (1 << 1));
    bool isBcdFormat = !(regB & (1 << 2));

    if (isBcdFormat) {
        second = ConvertFromBcd(second);
        minute = ConvertFromBcd(minute);
        hour = ConvertFromBcd(hour);
        day = ConvertFromBcd(day);
        month = ConvertFromBcd(month);
        year = ConvertFromBcd(year);
    }

    if (is24Hour && (hour & (1 << 7))) {
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
        year += 2000;
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
