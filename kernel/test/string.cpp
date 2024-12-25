#include <gtest/gtest.h>

#include "std/string_view.hpp"

using namespace stdx::literals;

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
