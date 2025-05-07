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

class IplTest : public testing::Test {
public:
    void SetUp() override {
        gIpl = km::Ipl::ePassive;
    }
};

#if 0
TEST(IplTest, Basic) {
    IplTag<ePassive> current = EnforceIpl<ePassive>();
    ASSERT_EQ(current.level(), ePassive);

    IplTag<eDispatch> dispatch = RaiseIpl<eDispatch>(std::move(current));
    IplTag<ePassive> passive = LowerIpl<ePassive>(std::move(dispatch));
    ASSERT_EQ(passive.level(), ePassive);

    ReleaseIpl(std::move(passive));
}
#endif

TEST_F(IplTest, Consumed) {
    IplTag<ePassive> current = EnforceIpl<ePassive>();
    IplTag<eDispatch> dispatch = RaiseIpl<eDispatch>(std::move(current));

    dispatch.level();

    ReleaseIpl(std::move(dispatch));
}

void ByReference(IplTag<eDispatch>& tag [[clang::param_typestate(unconsumed)]]) {
    ASSERT_EQ(tag.level(), eDispatch);
}

TEST_F(IplTest, ByReference) {
    IplTag<ePassive> current = EnforceIpl<ePassive>();
    IplTag<eDispatch> dispatch = RaiseIpl<eDispatch>(std::move(current));

    ByReference(dispatch);

    ReleaseIpl(std::move(dispatch));
}
