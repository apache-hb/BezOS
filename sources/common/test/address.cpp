#include <gtest/gtest.h>

#include "common/address.hpp"

struct TestAddressSpace {
    using Storage = uintptr_t;
};

struct TestAddress : public sm::Address<TestAddressSpace> {
    using Address::Address;
};

TEST(AddressTest, Construct) {
    TestAddress address = 0x12345678;
    EXPECT_TRUE(address == 0x12345678);
    EXPECT_EQ(address, 0x12345678);

    TestAddress address2 = nullptr;
    EXPECT_EQ(address2, 0);
}

TEST(AddressTest, Compare) {
    TestAddress a = 0x1000;
    TestAddress b = 0x2000;

    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_LE(a, b);
    EXPECT_GE(b, a);
    EXPECT_EQ(a, a);
    EXPECT_NE(a, b);
}

TEST(AddressTest, Null) {
    TestAddress address = 0x12345678;
    EXPECT_FALSE(address.isNull());

    TestAddress nullAddress(nullptr);
    EXPECT_TRUE(nullAddress.isNull());
}

TEST(AddressTest, IsAlignedTo) {
    TestAddress address = 0x1000;
    EXPECT_TRUE(address.isAlignedTo(0x1000));
    EXPECT_FALSE(address.isAlignedTo(0x2000));
}

TEST(AddressTest, Add) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = address + 0x100;
    EXPECT_EQ(address2, 0x1100);
}

TEST(AddressTest, AddEq) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    address += 0x100;
    EXPECT_EQ(address, 0x1100);
}

TEST(AddressTest, Sub) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = address - 0x100;
    EXPECT_EQ(address2, 0x0F00);
}

TEST(AddressTest, SubEq) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    address -= 0x100;
    EXPECT_EQ(address, 0x0F00);
}

TEST(AddressTest, Difference) {
    TestAddress address = 0x1000;
    TestAddress address2 = 0x1100;
    EXPECT_EQ(address, 0x1000);

    ptrdiff_t diff = address2 - address;
    EXPECT_EQ(diff, 0x100);
}

TEST(AddressTest, Modulo) {
    TestAddress address = 25;

    TestAddress rem = address % 10;
    EXPECT_EQ(rem, 5);
}

TEST(AddressTest, PostfixIncrement) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = address++;
    EXPECT_EQ(address2, 0x1000);
    EXPECT_EQ(address, 0x1001);
}

TEST(AddressTest, PostfixDecrement) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = address--;
    EXPECT_EQ(address2, 0x1000);
    EXPECT_EQ(address, 0x0FFF);
}

TEST(AddressTest, PrefixIncrement) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = ++address;
    EXPECT_EQ(address2, 0x1001);
    EXPECT_EQ(address, 0x1001);
}

TEST(AddressTest, PrefixDecrement) {
    TestAddress address = 0x1000;
    EXPECT_EQ(address, 0x1000);

    TestAddress address2 = --address;
    EXPECT_EQ(address2, 0x0FFF);
    EXPECT_EQ(address, 0x0FFF);
}
