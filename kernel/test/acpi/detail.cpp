#include <gtest/gtest.h>

#include "acpi/aml.hpp"

using namespace stdx::literals;

TEST(AmlDetailTest, PkgLengthEmpty) {
    uint8_t data[] = { 0x00 };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0);
}

TEST(AmlDetailTest, PkgLengthZero) {
    uint8_t data[1] = { };

    // purposefully an empty span
    std::span<const uint8_t> code(std::span(data, data));

    acpi::AmlParser parser(code);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0);
}

TEST(AmlDetailTest, PkgLengthOne) {
    uint8_t data[1] = { 0x01 };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 1);
}

TEST(AmlDetailTest, PkgLengthMax) {
    uint8_t data[4] = { 0b11001111, 0xFF, 0xFF, 0xFF };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0x0FFFFFFF);
}

TEST(AmlDetailTest, ByteDataValue) {
    uint8_t data[] = { 0x42 };
    acpi::AmlParser parser(data);
    uint8_t value = acpi::detail::ByteData(parser);

    ASSERT_EQ(value, 0x42);
}

TEST(AmlDetailTest, WordDataValue) {
    uint8_t data[] = { 0xFF, 0x00 };
    acpi::AmlParser parser(data);
    uint16_t value = acpi::detail::WordData(parser);

    ASSERT_EQ(value, 0x00FF);
}

TEST(AmlDetailTest, DwordDataValue) {
    uint8_t data[] = { 0x11, 0x22, 0x33, 0x44 };
    acpi::AmlParser parser(data);
    uint32_t value = acpi::detail::DwordData(parser);

    ASSERT_EQ(value, 0x44332211);
}

TEST(AmlDetailTest, QwordDataValue) {
    uint8_t data[] = {
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88
    };

    acpi::AmlParser parser(data);
    uint64_t value = acpi::detail::QwordData(parser);

    ASSERT_EQ(value, 0x8877665544332211);
}
