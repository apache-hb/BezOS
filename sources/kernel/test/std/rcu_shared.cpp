#include <gtest/gtest.h>

#include "std/rcu/atomic.hpp"
#include "std/rcu/weak.hpp"

class RcuSharedTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }
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

TEST_F(RcuSharedTest, Copy) {
    sm::RcuShared<int> shared(&domain, 42);
    sm::RcuShared<int> copy = shared;
    ASSERT_EQ(*copy.get(), 42);
    ASSERT_EQ(shared.strongCount(), 2);
    ASSERT_EQ(copy.strongCount(), 2);
}

TEST_F(RcuSharedTest, Weak) {
    sm::RcuShared<int> shared(&domain, 42);
    sm::RcuWeak<int> weak = shared;
    ASSERT_EQ(*weak.lock().get(), 42);
    ASSERT_GE(shared.strongCount(), 1);
    ASSERT_EQ(weak.weakCount(), 1);
}

TEST_F(RcuSharedTest, WeakOnly) {
    sm::RcuWeak<int> weak;

    {
        sm::RcuShared<int> shared(&domain, 42);
        weak = shared;
    }
    domain.synchronize();

    ASSERT_EQ(weak.lock(), nullptr);
    ASSERT_EQ(weak.weakCount(), 1);
}

TEST_F(RcuSharedTest, Store) {
    sm::RcuAtomic<int> atomic(&domain, 42);
    sm::RcuAtomic<int> atomic2(&domain, 100);

    sm::RcuShared<int> shared = atomic.load();
    atomic2.store(shared);
    ASSERT_EQ(*atomic2.load().get(), 42);
}

class SharedObject : public sm::RcuIntrusive<SharedObject> {
public:
    sm::RcuShared<SharedObject> getShared() {
        return loanShared();
    }
};

TEST_F(RcuSharedTest, Intrusive) {
    sm::RcuAtomic<SharedObject> atomic(&domain);
    sm::RcuShared<SharedObject> object = atomic.load();
    ASSERT_NE(object->getShared(), nullptr);
}
