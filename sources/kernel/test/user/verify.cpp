#include <gtest/gtest.h>

#include "user/context.hpp"
#include "setup.hpp"

class VerifyPointerTest : public testing::Test {
public:
    km::PageBuilder pm { 48, 48, km::GetDefaultPatLayout() };
};

TEST_F(VerifyPointerTest, DefaultRules) {
    km::VerifyRules rules{};

    ASSERT_EQ(km::VerifyUserPointer(rules, 0x1000, 0x1000, &pm), OsStatusSuccess);
}

TEST_F(VerifyPointerTest, UnalignedPointer) {
    km::VerifyRules rules {
        .alignment = 8,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0x1001, 0x1000, &pm), OsStatusInvalidAddress);
}

TEST_F(VerifyPointerTest, TooSmall) {
    km::VerifyRules rules {
        .alignment = 8,
        .minSize = 0x1000,
        .maxSize = 0x100000,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0x1000, 200, &pm), OsStatusInvalidSpan);
}

TEST_F(VerifyPointerTest, TooLarge) {
    km::VerifyRules rules {
        .alignment = 8,
        .minSize = 0x1000,
        .maxSize = 0x100000,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0x1000, rules.maxSize + 1, &pm), OsStatusInvalidSpan);
}

TEST_F(VerifyPointerTest, NotCanonical) {
    km::VerifyRules rules {
        .alignment = 8,
        .minSize = 0x1000,
        .maxSize = 0x100000,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0xFAFA'0000'0000'1000, 0x4000, &pm), OsStatusInvalidAddress);
}

TEST_F(VerifyPointerTest, HigherHalf) {
    km::VerifyRules rules {
        .alignment = 8,
        .minSize = 0x1000,
        .maxSize = 0x100000,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0xFFFF'0000'0000'1000, 0x4000, &pm), OsStatusInvalidAddress);
}

TEST_F(VerifyPointerTest, WrapAround) {
    km::VerifyRules rules {
        .alignment = 8,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0xFFFF'FFFF'FFFF'0000, 0xFFFF'FFFF, &pm), OsStatusInvalidSpan);
}

TEST_F(VerifyPointerTest, InvalidMultiple) {
    km::VerifyRules rules {
        .alignment = 8,
        .multiple = 64,
    };

    ASSERT_EQ(km::VerifyUserPointer(rules, 0x2000, 65, &pm), OsStatusInvalidSpan);
}

TEST_F(VerifyPointerTest, NegativeSpan) {
    km::VerifyRules rules {
        .alignment = 8,
        .minSize = 0x1000,
        .maxSize = 0x100000,
    };

    ASSERT_EQ(km::VerifyUserRange(rules, 0x5000, 0x4000, &pm), OsStatusInvalidSpan);
}
