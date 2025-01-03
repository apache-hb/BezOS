#include <gtest/gtest.h>

#include "crt.hpp"

TEST(CrtTest, MemoryCopySmall) {
    uint8_t buffer[3] = { 0x1, 0x2, 0x3 };
    uint8_t copy[3] = { 0x0, 0x0, 0x0 };
    KmMemoryCopy(copy, buffer, sizeof(copy));

    ASSERT_EQ(copy[0], 0x1);
    ASSERT_EQ(copy[1], 0x2);
    ASSERT_EQ(copy[2], 0x3);
}

TEST(CrtTest, MemoryCopy8) {
    uint64_t src = 0x123456789ABCDEF0;
    uint64_t dst = 0;
    KmMemoryCopy(&dst, &src, sizeof(dst));

    ASSERT_EQ(dst, src);
}

TEST(CrtTest, MemoryCopyLarge) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy, buffer, sizeof(buffer));

    for (size_t i = 0; i < sizeof(buffer); i++) {
        ASSERT_EQ(copy[i], buffer[i]);
    }
}

struct CrtCopyUnalignedTest
    : public testing::TestWithParam<int> {};

INSTANTIATE_TEST_SUITE_P(
    Unaligned, CrtCopyUnalignedTest,
    testing::Values(1, 2, 3, 4, 5, 6, 7));

TEST_P(CrtCopyUnalignedTest, MemoryCopyUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }

    int offset = GetParam();

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy + offset, buffer, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i + offset], buffer[i]);
    }
}

TEST_P(CrtCopyUnalignedTest, MemoryCopyDstUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }

    int offset = GetParam();

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy, buffer + offset, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i], buffer[i + offset]);
    }
}

TEST_P(CrtCopyUnalignedTest, MemoryCopyBothUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }

    int offset = GetParam();
    int otherOffset = 8 - offset;

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy + offset, buffer + otherOffset, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i + offset], buffer[i + otherOffset]);
    }
}
