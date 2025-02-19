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
        buffer[i] = i % 256;
    }

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy, buffer, sizeof(buffer));

    for (size_t i = 0; i < sizeof(buffer); i++) {
        ASSERT_EQ(copy[i], buffer[i]);
    }
}

TEST(CrtTest, MemoryCopySlop) {
    uint8_t buffer[16] = { 0x0 };
    buffer[4] = 0x1;
    uint8_t copy[16] = { 0x0 };
    KmMemoryCopy(copy + 4, buffer + 4, 4);

    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQ(copy[i], 0x0);
    }

    for (size_t i = 4; i < 8; i++) {
        ASSERT_EQ(copy[i], buffer[i]);
    }

    for (size_t i = 8; i < sizeof(buffer); i++) {
        ASSERT_EQ(copy[i], 0x0);
    }
}

struct CrtUnalignedTest
    : public testing::TestWithParam<int> {};

INSTANTIATE_TEST_SUITE_P(
    Unaligned, CrtUnalignedTest,
    testing::Values(1, 2, 3, 4, 5, 6, 7));

TEST_P(CrtUnalignedTest, MemoryCopyUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }

    int offset = GetParam();

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy + offset, buffer, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i + offset], buffer[i]);
    }
}

TEST_P(CrtUnalignedTest, MemoryCopyDstUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }

    int offset = GetParam();

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy, buffer + offset, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i], buffer[i + offset]);
    }
}

TEST_P(CrtUnalignedTest, MemoryCopyBothUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }

    int offset = GetParam();
    int otherOffset = 8 - offset;

    uint8_t copy[sizeof(buffer)];
    KmMemoryCopy(copy + offset, buffer + otherOffset, sizeof(buffer) - std::max(otherOffset, offset));

    for (size_t i = 0; i < sizeof(buffer) - std::max(otherOffset, offset); i++) {
        ASSERT_EQ(copy[i + offset], buffer[i + otherOffset]);
    }
}

TEST(CrtTest, MemorySetSlop) {
    uint8_t value[16] = { 0x0 };
    KmMemorySet(value + 4, 0xFF, 4);

    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQ(value[i], 0x0);
    }

    for (size_t i = 4; i < 8; i++) {
        ASSERT_EQ(value[i], 0xFF);
    }

    for (size_t i = 8; i < sizeof(value); i++) {
        ASSERT_EQ(value[i], 0x0);
    }
}

TEST(CrtTest, MemorySetSmall) {
    uint8_t copy[3] = { 0x0, 0x0, 0x0 };
    KmMemorySet(copy, 0xFF, sizeof(copy));

    ASSERT_EQ(copy[0], 0xFF);
    ASSERT_EQ(copy[1], 0xFF);
    ASSERT_EQ(copy[2], 0xFF);
}

TEST(CrtTest, MemorySet8) {
    uint64_t dst = 0x123456789ABCDEF0;
    uint64_t expected = 0x0101010101010101;
    KmMemorySet(&dst, 0x01, sizeof(dst));

    ASSERT_EQ(dst, expected);
}

TEST(CrtTest, MemorySetLarge) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }

    uint8_t copy[sizeof(buffer)];
    KmMemorySet(copy, 0xFF, sizeof(copy));

    for (size_t i = 0; i < sizeof(copy); i++) {
        ASSERT_EQ(copy[i], 0xFF);
    }
}

TEST_P(CrtUnalignedTest, MemorySetUnaligned) {
    uint8_t buffer[1024];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }

    int offset = GetParam();

    uint8_t copy[sizeof(buffer)];
    KmMemorySet(copy + offset, 0xFF, sizeof(buffer) - offset);

    for (size_t i = 0; i < sizeof(buffer) - offset; i++) {
        ASSERT_EQ(copy[i + offset], 0xFF);
    }
}
