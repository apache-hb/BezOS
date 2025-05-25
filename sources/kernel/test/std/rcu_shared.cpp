#include <gtest/gtest.h>

#include "std/rcu/atomic.hpp"

class RcuSharedTest : public testing::Test {
public:
    sm::RcuDomain domain;
};

TEST_F(RcuSharedTest, CompareExchangeSuccess) {
    sm::RcuAtomic<int> atomic(&domain, 42);
    sm::RcuShared<int> expected = atomic.load();
    sm::RcuShared<int> desired = sm::RcuShared<int>(&domain, 100);
    ASSERT_TRUE(atomic.compare_exchange_weak(expected, desired));
}

TEST_F(RcuSharedTest, CompareExchangeFail) {
    sm::RcuAtomic<int> atomic(&domain, 42);
    sm::RcuShared<int> expected = sm::RcuShared<int>(&domain, 100);
    sm::RcuShared<int> desired = atomic.load();
    ASSERT_FALSE(atomic.compare_exchange_weak(expected, desired));
    ASSERT_EQ(*expected.get(), 42);
}
