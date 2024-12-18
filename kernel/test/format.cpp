#include <gtest/gtest.h>

#include "kernel.hpp"

struct FormatTest : public testing::TestWithParam<std::tuple<int, int, stdx::StringView>> {};

INSTANTIATE_TEST_SUITE_P(
    IntFormat, FormatTest,
    testing::Values(
        std::make_tuple(0, 10, stdx::StringView("0")),
        std::make_tuple(1, 10, stdx::StringView("1")),
        std::make_tuple(10, 10, stdx::StringView("10")),
        std::make_tuple(15, 10, stdx::StringView("15")),
        std::make_tuple(16, 10, stdx::StringView("16")),
        std::make_tuple(2147483647, 10, stdx::StringView("2147483647")),
        std::make_tuple(-2147483648, 10, stdx::StringView("-2147483648"))
    ),
    [](const auto& info) {
        std::string result = std::format("{}_{}", std::get<0>(info.param), std::get<1>(info.param));
        std::replace(result.begin(), result.end(), '-', 'n');
        return result;
    });

TEST_P(FormatTest, IntFormat) {
    auto [value, base, expected] = GetParam();
    char buffer[64];
    stdx::StringView result = FormatInt(buffer, value, base);
    ASSERT_EQ(std::string_view(result), std::string_view(expected)) << std::string_view(result);
}

struct PaddingTest : public testing::TestWithParam<std::tuple<int, char, stdx::StringView>> {};

INSTANTIATE_TEST_SUITE_P(
    IntPadding, PaddingTest,
    testing::Values(
        std::make_tuple(0, '0', stdx::StringView("0000000000")),
        std::make_tuple(1, '0', stdx::StringView("0000000001")),
        std::make_tuple(10, '0', stdx::StringView("0000000010")),
        std::make_tuple(15, '0', stdx::StringView("0000000015")),
        std::make_tuple(16, '0', stdx::StringView("0000000016")),
        std::make_tuple(2147483647, '0', stdx::StringView("2147483647")),
        std::make_tuple(-2147483648, '0', stdx::StringView("-2147483648"))
    ),
    [](const auto& info) {
        std::string result = std::format("{}_{}", std::get<0>(info.param), std::get<1>(info.param));
        std::replace(result.begin(), result.end(), '-', 'n');
        return result;
    });

TEST_P(PaddingTest, IntPadding) {
    auto [value, fill, expected] = GetParam();
    char buffer[64];
    stdx::StringView result = FormatInt(buffer, value, 10, 10, fill);
    ASSERT_EQ(std::string_view(result), std::string_view(expected)) << std::string_view(result);
}
