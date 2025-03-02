#include <gtest/gtest.h>

#include "gdt.hpp"

using namespace stdx::literals;

TEST(GdtTest, FormatNull) {
    x64::GdtEntry entry = x64::GdtEntry::null();
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x0000000000000000 [ NULL ]"_sv) << std::string_view(result);
}

TEST(GdtTest, FormatRealModeCode) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eRealMode, x64::Access::eCode, 0xFFFF);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00009B000000FFFF [ LIMIT = 0x00FFFF ACCESS = [ CODE/DATA EXECUTABLE READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0xFFFF);
    EXPECT_EQ(entry.flags(), x64::Flags::eRealMode);
    EXPECT_EQ(entry.access(), x64::Access::eCode);
    EXPECT_EQ(entry.value(), 0x9b000000ffff);
}

TEST(GdtTest, FormatRealModeData) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eRealMode, x64::Access::eData, 0xFFFF);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x000093000000FFFF [ LIMIT = 0x00FFFF ACCESS = [ CODE/DATA READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0xFFFF);
    EXPECT_EQ(entry.flags(), x64::Flags::eRealMode);
    EXPECT_EQ(entry.access(), x64::Access::eData);
    EXPECT_EQ(entry.value(), 0x93000000ffff);
}

TEST(GdtTest, FormatProtectedModeCode) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eProtectedMode, x64::Access::eCode, 0xFFFF);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00C09B000000FFFF [ LIMIT = 0x00FFFF FLAGS = [ SIZE GRANULARITY ] ACCESS = [ CODE/DATA EXECUTABLE READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0xFFFF);
    EXPECT_EQ(entry.flags(), x64::Flags::eProtectedMode);
    EXPECT_EQ(entry.access(), x64::Access::eCode);
    EXPECT_EQ(entry.value(), 0xc09b000000ffff);
}

TEST(GdtTest, FormatProtectedModeData) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eProtectedMode, x64::Access::eData, 0);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00C0930000000000 [ FLAGS = [ SIZE GRANULARITY ] ACCESS = [ CODE/DATA READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0);
    EXPECT_EQ(entry.flags(), x64::Flags::eProtectedMode);
    EXPECT_EQ(entry.access(), x64::Access::eData);
    EXPECT_EQ(entry.value(), 0xc0930000000000);
}

TEST(GdtTest, FormatLongModeCode) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eLong | x64::Flags::eGranularity, x64::Access::eCode, 0);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00A09B0000000000 [ FLAGS = [ LONG GRANULARITY ] ACCESS = [ CODE/DATA EXECUTABLE READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0);
    EXPECT_EQ(entry.flags(), x64::Flags::eLong | x64::Flags::eGranularity);
    EXPECT_EQ(entry.access(), x64::Access::eCode);
    EXPECT_EQ(entry.value(), 0xa09b0000000000);
}

TEST(GdtTest, FormatLongModeData) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eLong | x64::Flags::eGranularity, x64::Access::eData, 0);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00A0930000000000 [ FLAGS = [ LONG GRANULARITY ] ACCESS = [ CODE/DATA READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0);
    EXPECT_EQ(entry.flags(), x64::Flags::eLong | x64::Flags::eGranularity);
    EXPECT_EQ(entry.access(), x64::Access::eData);
    EXPECT_EQ(entry.value(), 0xa0930000000000);
}

TEST(GdtTest, FormatAll) {
    x64::Flags flags = x64::Flags::eLong | x64::Flags::eSize | x64::Flags::eGranularity;
    x64::Access access
        = x64::Access::eAccessed
        | x64::Access::eReadWrite
        | x64::Access::eEscalateDirection
        | x64::Access::eExecutable
        | x64::Access::eCodeOrDataSegment
        | x64::Access::eRing3
        | x64::Access::ePresent;

    x64::GdtEntry entry = x64::GdtEntry(flags, access, 0xFFFFF);
    auto result = km::format(entry);

    EXPECT_EQ(result, "0x00EFFF000000FFFF [ LIMIT = 0x0FFFFF FLAGS = [ LONG SIZE GRANULARITY ] ACCESS = [ CODE/DATA EXECUTABLE READ/WRITE PRESENT ACCESSED DIRECTION ] RING 3 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0);
    EXPECT_EQ(entry.limit(), 0xFFFFF);
    EXPECT_EQ(entry.flags(), flags);
    EXPECT_EQ(entry.access(), access);
}

TEST(GdtTest, FormatBase) {
    x64::GdtEntry entry = x64::GdtEntry(x64::Flags::eRealMode, x64::Access::eData, 0xFFFF, 0xFFFF);
    auto result = km::format(entry);
    EXPECT_EQ(result, "0x00009300FFFFFFFF [ LIMIT = 0x00FFFF BASE = 0x0000FFFF ACCESS = [ CODE/DATA READ/WRITE PRESENT ACCESSED ] RING 0 ]"_sv) << std::string_view(result);

    EXPECT_EQ(entry.base(), 0xFFFF);
    EXPECT_EQ(entry.limit(), 0xFFFF);
    EXPECT_EQ(entry.flags(), x64::Flags::eRealMode);
    EXPECT_EQ(entry.access(), x64::Access::eData);
    EXPECT_EQ(entry.value(), 0x00009300FFFFFFFF);
}
