#include <gtest/gtest.h>

#include "ipl.hpp"

using namespace km;
using enum km::Ipl;

static thread_local km::Ipl gIpl = km::Ipl::ePassive;

void km::detail::EnforceIpl(km::Ipl level) noexcept {
    ASSERT_TRUE(gIpl >= level);
}

void km::detail::RaiseIpl(km::Ipl from, km::Ipl to) noexcept {
    ASSERT_TRUE(from < to);
    ASSERT_EQ(gIpl, from);
    gIpl = to;
}

void km::detail::LowerIpl(km::Ipl from, km::Ipl to) noexcept {
    ASSERT_TRUE(from > to);
    ASSERT_EQ(gIpl, from);
    gIpl = to;
}

TEST(IplTest, Basic) {
    IplTag<ePassive> current = EnforceIpl<ePassive>();
    ASSERT_EQ(current.level(), ePassive);

    IplTag<eDispatch> dispatch = RaiseIpl<eDispatch>(std::move(current));
    IplTag<ePassive> passive = LowerIpl<ePassive>(std::move(dispatch));
    ASSERT_EQ(passive.level(), ePassive);
}
