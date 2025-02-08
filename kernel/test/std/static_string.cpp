#include <gtest/gtest.h>

#include "std/static_string.hpp"
#include "std/string_view.hpp"

using namespace stdx::literals;

TEST(StaticStringTest, Convert) {
    stdx::StaticString<16> str = "Hello, World!";
    ASSERT_EQ(str.count(), 13);
}

TEST(StaticStringTest, FromStringView) {
    stdx::StringView view = "Hello, World!"_sv;

    stdx::StaticString<16> str = view;
    ASSERT_EQ(str.count(), 13);
}
