#include <gtest/gtest.h>

#include "cmos.hpp"

using namespace km;

TEST(CmosTest, ConvertBcd) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x45,
        .minute = 0x59,
        .hour = 0x23,
        .day = 0x31,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 59);
    ASSERT_EQ(date.hour, 23);
    ASSERT_EQ(date.day, 31);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, ConvertBcd12Hour) {
    detail::CmosRegisters regs = {
        .regB = k12HourClock,
        .second = 0x45,
        .minute = 0x59,
        .hour = 0x11,
        .day = 0x31,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 59);
    ASSERT_EQ(date.hour, 11);
    ASSERT_EQ(date.day, 31);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, ConvertBcd12HourHighHour) {
    detail::CmosRegisters regs = {
        .regB = k12HourClock,
        .second = 0x45,
        .minute = 0x59,
        .hour = 0x7 | (1 << 7),
        .day = 0x31,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 59);
    ASSERT_EQ(date.hour, 19);
    ASSERT_EQ(date.day, 31);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, ConvertBcd24Hour) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x45,
        .minute = 0x59,
        .hour = 0x7,
        .day = 0x31,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 59);
    ASSERT_EQ(date.hour, 7);
    ASSERT_EQ(date.day, 31);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, HourRollover) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x45,
        .minute = 0x59,
        .hour = 0x25,
        .day = 0x28,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 59);
    ASSERT_EQ(date.hour, 1);
    ASSERT_EQ(date.day, 29);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, MinuteRollover) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x45,
        .minute = 0x61,
        .hour = 0x22,
        .day = 0x28,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 45);
    ASSERT_EQ(date.minute, 1);
    ASSERT_EQ(date.hour, 23);
    ASSERT_EQ(date.day, 28);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, SecondRollover) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x62,
        .minute = 0x50,
        .hour = 0x20,
        .day = 0x28,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 2);
    ASSERT_EQ(date.minute, 51);
    ASSERT_EQ(date.hour, 20);
    ASSERT_EQ(date.day, 28);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}

TEST(CmosTest, MultiRollover) {
    detail::CmosRegisters regs = {
        .regB = 0,
        .second = 0x62,
        .minute = 0x62,
        .hour = 0x20,
        .day = 0x28,
        .month = 0x12,
        .year = 0x99,
        .century = 0,
    };

    DateTime date = detail::ConvertCmosToDate(regs);
    ASSERT_EQ(date.second, 2);
    ASSERT_EQ(date.minute, 3);
    ASSERT_EQ(date.hour, 21);
    ASSERT_EQ(date.day, 28);
    ASSERT_EQ(date.month, 12);
    ASSERT_EQ(date.year, kDefaultYear + 99);
}
