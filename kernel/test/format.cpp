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
