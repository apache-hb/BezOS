#include <gtest/gtest.h>

#include "pat.hpp"

TEST(PageAttributeTableTest, WriteBounds) {
    uint64_t pat = 0;

    uint64_t before = pat;

    x64::detail::SetPatEntry(pat, 0, km::MemoryType::eUncachedOverridable);

    ASSERT_EQ(x64::detail::GetPatEntry(pat, 0), km::MemoryType::eUncachedOverridable);

    for (int i = 1; i < 8; i++) {
        ASSERT_EQ(x64::detail::GetPatEntry(pat, i), x64::detail::GetPatEntry(before, i));
    }
}

TEST(PageAttributeTableTest, WriteAll) {
    uint64_t pat = 0;

    for (int i = 0; i < 8; i++) {
        x64::detail::SetPatEntry(pat, i, km::MemoryType::eUncachedOverridable);
    }

    for (int i = 0; i < 8; i++) {
        ASSERT_EQ(x64::detail::GetPatEntry(pat, i), km::MemoryType::eUncachedOverridable);
    }
}

struct PageAttributeTableTest
    : public testing::TestWithParam<std::tuple<uint8_t, km::MemoryType, uint8_t, km::MemoryType>> {};

INSTANTIATE_TEST_SUITE_P(
    Modify, PageAttributeTableTest,
    testing::Combine(
        testing::Values(0, 1, 2, 3, 4, 5, 6, 7),
        testing::Values(
            km::MemoryType::eUncached,
            km::MemoryType::eWriteCombine,
            km::MemoryType::eWriteThrough,
            km::MemoryType::eWriteBack,
            km::MemoryType::eWriteProtect,
            km::MemoryType::eUncachedOverridable
        ),
        testing::Values(0, 1, 2, 3, 4, 5, 6, 7),
        testing::Values(
            km::MemoryType::eUncached,
            km::MemoryType::eWriteCombine,
            km::MemoryType::eWriteThrough,
            km::MemoryType::eWriteBack,
            km::MemoryType::eWriteProtect,
            km::MemoryType::eUncachedOverridable
        )
    ),
    [](const auto& info) {
        auto type = km::format(std::get<1>(info.param));
        auto type1 = km::format(std::get<3>(info.param));
        auto out = std::format("{}_{}_{}_{}", std::get<0>(info.param), std::get<2>(info.param), std::string_view(type), std::string_view(type1));
        std::replace(out.begin(), out.end(), ' ', '_');
        std::replace(out.begin(), out.end(), '-', 'n');
        return out;
    });

TEST_P(PageAttributeTableTest, Modify) {
    auto [index0, type0, index1, type1] = GetParam();

    if (index0 == index1) {
        return;
    }

    uint64_t pat = 0;

    x64::detail::SetPatEntry(pat, index0, type0);
    x64::detail::SetPatEntry(pat, index1, type1);

    ASSERT_EQ(x64::detail::GetPatEntry(pat, index0), type0);
    ASSERT_EQ(x64::detail::GetPatEntry(pat, index1), type1);
}
