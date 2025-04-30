#include <gtest/gtest.h>

#include "common/address.hpp"

struct TestAddressSpace {
    using Storage = uintptr_t;
};

struct TestAddress : public sm::Address<TestAddressSpace> {
    using Address::Address;
};

TEST(AddressTest, Construct) {
    TestAddress address(0x12345678);
    EXPECT_EQ(address, 0x12345678);
}
