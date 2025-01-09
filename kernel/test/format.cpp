#include <gtest/gtest.h>

#include "util/format.hpp"

using namespace stdx::literals;

struct FormatTest : public testing::TestWithParam<std::tuple<int, int, stdx::StringView>> {};

INSTANTIATE_TEST_SUITE_P(
    IntFormat, FormatTest,
    testing::Values(
        std::make_tuple(0, 10, "0"_sv),
        std::make_tuple(1, 10,  "1"_sv),
        std::make_tuple(10, 10, "10"_sv),
        std::make_tuple(15, 10, "15"_sv),
        std::make_tuple(16, 10, "16"_sv),
        std::make_tuple(2147483647, 10, "2147483647"_sv),
        std::make_tuple(-2147483648, 10, "-2147483648"_sv)
    ),
    [](const auto& info) {
        std::string result = std::format("{}_{}", std::get<0>(info.param), std::get<1>(info.param));
        std::replace(result.begin(), result.end(), '-', 'n');
        return result;
    });

TEST_P(FormatTest, IntFormat) {
    auto [value, base, expected] = GetParam();
    char buffer[64];
    stdx::StringView result = km::FormatInt(buffer, value, base);
    ASSERT_EQ(std::string_view(result), std::string_view(expected)) << std::string_view(result);
}

struct PaddingTest : public testing::TestWithParam<std::tuple<int, char, stdx::StringView>> {};

INSTANTIATE_TEST_SUITE_P(
    IntPadding, PaddingTest,
    testing::Values(
        std::make_tuple(0, '0',  "0000000000"_sv),
        std::make_tuple(1, '0',  "0000000001"_sv),
        std::make_tuple(10, '0', "0000000010"_sv),
        std::make_tuple(15, '0', "0000000015"_sv),
        std::make_tuple(16, '0', "0000000016"_sv),
        std::make_tuple(2147483647, '0', "2147483647"_sv),
        std::make_tuple(-2147483648, '0', "-2147483648"_sv)
    ),
    [](const auto& info) {
        std::string result = std::format("{}_{}", std::get<0>(info.param), std::get<1>(info.param));
        std::replace(result.begin(), result.end(), '-', 'n');
        return result;
    });

TEST_P(PaddingTest, IntPadding) {
    auto [value, fill, expected] = GetParam();
    char buffer[64];
    stdx::StringView result = km::FormatInt(buffer, value, 10, 10, fill);
    ASSERT_EQ(std::string_view(result), std::string_view(expected)) << std::string_view(result);
}

struct StringStream : public km::IOutStream {
    std::string buffer;

    void write(stdx::StringView message) override {
        buffer.append(std::string_view(message));
    }
};

static constexpr uint8_t kData[] = {
    0x41, 0x50, 0x49, 0x43, 0x78, 0x00, 0x00, 0x00, 0x03, 0x88, 0x42, 0x4F, 0x43, 0x48, 0x53, 0x20,
    0x42, 0x58, 0x50, 0x43, 0x20, 0x20, 0x20, 0x20, 0x01, 0x00, 0x00, 0x00, 0x42, 0x58, 0x50, 0x43,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x0A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0A, 0x00, 0x05, 0x05, 0x00,
    0x00, 0x00, 0x0D, 0x00, 0x02, 0x0A, 0x00, 0x09, 0x09, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x02, 0x0A,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x02, 0x0A, 0x00, 0x0B, 0x0B, 0x00, 0x00, 0x00,
    0x0D, 0x00, 0x04, 0x06, 0xFF, 0x00, 0x00, 0x01,
};

static constexpr stdx::StringView kHexDump = stdx::StringView::ofString(
"0xFFFF800007FE21E8 : 4150 4943 7800 0000 0388 424F 4348 5320   APICx.....BOCHS \n"
"0xFFFF800007FE21F8 : 4258 5043 2020 2020 0100 0000 4258 5043   BXPC    ....BXPC\n"
"0xFFFF800007FE2208 : 0100 0000 0000 E0FE 0100 0000 0008 0000   ................\n"
"0xFFFF800007FE2218 : 0100 0000 010C 0000 0000 C0FE 0000 0000   ................\n"
"0xFFFF800007FE2228 : 020A 0000 0200 0000 0000 020A 0005 0500   ................\n"
"0xFFFF800007FE2238 : 0000 0D00 020A 0009 0900 0000 0D00 020A   ................\n"
"0xFFFF800007FE2248 : 000A 0A00 0000 0D00 020A 000B 0B00 0000   ................\n"
"0xFFFF800007FE2258 : 0D00 0406 FF00 0001                       ........        "
);

TEST(FormatTest, HexDumpFormat) {
    std::span<const uint8_t> data(kData);
    StringStream dst;
    dst.format(km::HexDump(data, 0xFFFF800007FE21E8));

    ASSERT_EQ(dst.buffer, std::string_view(kHexDump));
}
