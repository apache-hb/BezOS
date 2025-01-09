#include <gtest/gtest.h>

#include "acpi/aml.hpp"

using namespace stdx::literals;
using namespace acpi::literals;

TEST(AmlNameTest, Literal) {
    acpi::AmlName name = "PCID"_aml;

    ASSERT_EQ(name.prefix, 0);
    ASSERT_EQ(name.segments.count(), 1);
    ASSERT_EQ(name.segments[0], stdx::StaticString<4>("PCID"));
}

TEST(AmlNameTest, LiteralPrefix) {
    acpi::AmlName name = "\\_PR"_aml;

    ASSERT_EQ(name.prefix, 1);
    ASSERT_EQ(name.segments.count(), 1);
    ASSERT_EQ(name.segments[0], stdx::StaticString<4>("_PR_"));
}

TEST(AmlNameTest, DualNameSegLiteral) {
    acpi::AmlName name = "\\_PR.P001"_aml;

    ASSERT_EQ(name.prefix, 1);
    ASSERT_EQ(name.segments.count(), 2);
    ASSERT_EQ(name.segments[0], stdx::StaticString<4>("_PR_"));
    ASSERT_EQ(name.segments[1], stdx::StaticString<4>("P001"));
}

TEST(AmlNameTest, SingleName) {
    uint8_t data[] = { 'P', 'C', 'I', 'D' };
    acpi::AmlParser parser(data);
    acpi::AmlName name = acpi::detail::NameString(parser);

    ASSERT_EQ(name.prefix, 0);
    ASSERT_EQ(name.segments.count(), 1);
    ASSERT_EQ(name.segments[0], stdx::StaticString<4>("PCID"));

    ASSERT_EQ(name, "PCID"_aml);
}

TEST(AmlNameTest, DualName) {
    unsigned char data[] =
    {
        0x2e, 0x5f, 0x50, 0x52, 0x5f, 0x50, 0x30,
        0x30, 0x31, 0x08, 0x5f, 0x50, 0x43, 0x54, 0x12,
    };

    acpi::AmlParser parser(data);
    acpi::AmlName name = acpi::detail::NameString(parser);

    ASSERT_EQ(name.prefix, 1);
    ASSERT_EQ(name.segments.count(), 2);
    ASSERT_EQ(name.segments[0], stdx::StaticString<4>("_PR_"));
    ASSERT_EQ(name.segments[1], stdx::StaticString<4>("P001"));

    ASSERT_EQ(name, "\\_PR.P001"_aml);
}
