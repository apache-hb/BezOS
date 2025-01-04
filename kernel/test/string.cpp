#include <gtest/gtest.h>

#include "std/static_string.hpp"
#include "std/string_view.hpp"
#include "std/traits.hpp"

using namespace stdx::literals;

static_assert(requires { stdx::IsRange<const char, stdx::StringView>; });

static_assert(std::ranges::contiguous_range<stdx::StringView>);
static_assert(std::ranges::sized_range<stdx::StringView>);

TEST(StringViewTest, Length) {
    stdx::StringView view = "Hello, World!";
    ASSERT_EQ(view.count(), 14);
}

TEST(StringViewTest, Literals) {
    stdx::StringView view = "Hello, World!"_sv;
    ASSERT_EQ(view.count(), 13);
}

TEST(StringViewTest, ArrayLength) {
    char array[13] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
    stdx::StringView view = array;
    ASSERT_EQ(view.count(), 13);
}

TEST(StringViewTest, Iterator) {
    stdx::StringView view = "Hello, World!"_sv;

    ASSERT_EQ(std::begin(view), view.begin());
}

TEST(StaticStringTest, Convert) {
    stdx::StaticString<16> str = "Hello, World!";
    ASSERT_EQ(str.count(), 14);
}

TEST(StaticStringTest, FromStringView) {
    stdx::StringView view = "Hello, World!"_sv;

    stdx::StaticString<16> str = view;
    ASSERT_EQ(str.count(), 13);
}
