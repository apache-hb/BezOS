#include <gtest/gtest.h>

#include "arch/cr0.hpp"
#include "arch/cr4.hpp"

using namespace stdx::literals;

TEST(CrTest, Cr0FormatEmpty) {
    x64::Cr0 cr0 = x64::Cr0::of(0x00000000);
    auto result = km::format(cr0);

    ASSERT_EQ(result, "0x00000000 [ ]"_sv) << std::string_view(result);
}

TEST(CrTest, Cr0Format) {
    x64::Cr0 cr0 = x64::Cr0::of(0x60000031);
    auto result = km::format(cr0);

    ASSERT_EQ(result, "0x60000031 [ CD NW NE ET PE ]"_sv) << std::string_view(result);
}

TEST(CrTest, Cr0FormatAll) {
    x64::Cr0 cr0 = x64::Cr0::of(0xFFFFFFFF);
    auto result = km::format(cr0);

    ASSERT_TRUE(result.endsWith("]"_sv)) << std::string_view(result);
}

TEST(CrTest, Cr4FormatEmpty) {
    x64::Cr4 cr4 = x64::Cr4::of(0x00000000);
    auto result = km::format(cr4);

    ASSERT_EQ(result, "0x00000000 [ ]"_sv) << std::string_view(result);
}

TEST(CrTest, Cr4Format) {
    x64::Cr4 cr4 = x64::Cr4::of(0x00000031);
    auto result = km::format(cr4);

    ASSERT_EQ(result, "0x00000031 [ VME PSE PAE ]"_sv) << std::string_view(result);
}

TEST(CrTest, Cr4FormatAll) {
    x64::Cr4 cr4 = x64::Cr4::of(0xFFFFFFFF);
    auto result = km::format(cr4);

    ASSERT_TRUE(result.endsWith("]"_sv)) << std::string_view(result);
}

// If more flags are added, these tests will fail.
// When adding more flags update these to match the new count.
// This is to ensure that the strings max size is always large enough.

TEST(CrTest, Cr4StringSize) {
    x64::Cr4 cr4 = x64::Cr4::of(0xFFFFFFFF);
    auto result = km::format(cr4);
    ASSERT_EQ(result.count(), 127);
}

TEST(CrTest, Cr0StringSize) {
    x64::Cr0 cr0 = x64::Cr0::of(0xFFFFFFFF);
    auto result = km::format(cr0);
    ASSERT_EQ(result.count(), 47);
}
