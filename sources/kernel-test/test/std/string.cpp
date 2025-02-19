#include <gtest/gtest.h>

#include "std/string.hpp"

using namespace stdx::literals;

TEST(StringTest, Append) {
    stdx::String str;

    str.append("Hello");

    ASSERT_EQ(str.count(), 5);
}
