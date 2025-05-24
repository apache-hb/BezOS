#include <gtest/gtest.h>

#include "std/detail/counted.hpp"

using sm::detail::CountedObject;

TEST(RetireSlotsTest, Construct) {
    CountedObject<int> object { 0 };
    EXPECT_EQ(object.strongCount(), 1);
    EXPECT_EQ(object.weakCount(), 1);
}


TEST(RetireSlotsTest, Retire) {
    CountedObject<int> object { 0 };
    EXPECT_EQ(object.strongCount(), 1);
    EXPECT_EQ(object.weakCount(), 1);

    ASSERT_EQ(object.releaseStrong(1), sm::detail::EjectAction::eDestroy);
    EXPECT_EQ(object.strongCount(), 0);
    EXPECT_EQ(object.weakCount(), 1);
}

TEST(RetireSlotsTest, Retain) {
    CountedObject<int> object { 0 };
    EXPECT_EQ(object.strongCount(), 1);
    EXPECT_EQ(object.weakCount(), 1);

    ASSERT_TRUE(object.retainStrong(1));

    ASSERT_EQ(object.releaseStrong(1), sm::detail::EjectAction::eNone);
    EXPECT_EQ(object.strongCount(), 1);
    EXPECT_EQ(object.weakCount(), 1);
}
