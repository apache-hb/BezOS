#include <gtest/gtest.h>

#include "std/string_view.hpp"
#include "std/traits.hpp"

using namespace stdx::literals;

static_assert(requires { stdx::IsRange<const char, stdx::StringView>; });

static_assert(std::ranges::contiguous_range<stdx::StringView>);
static_assert(std::ranges::sized_range<stdx::StringView>);

TEST(StringViewTest, Length) {
    stdx::StringView view = "Hello, World!";
    ASSERT_EQ(view.count(), 13);
}

TEST(StringViewTest, Literals) {
    stdx::StringView view = "Hello, World!"_sv;
    ASSERT_EQ(view.count(), 13);
}

TEST(StringViewTest, ArrayLength) {
    std::array<char, 13> array = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
    stdx::StringView view = array;
    ASSERT_EQ(view.count(), 13);
}

TEST(StringViewTest, Iterator) {
    stdx::StringView view = "Hello, World!"_sv;

    ASSERT_EQ(std::begin(view), view.begin());
}
