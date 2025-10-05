#include <ktest/ktest.h>

#include "logger/categories.hpp"

class KernelStartupTest : public km::testing::Test {
public:
    void SetUp() override {

    }

    void TearDown() override {

    }
};

KTEST_F(KernelStartupTest, BasicTest) {
    TestLog.infof("Kernel startup test is running.");
    KASSERT_EQ(1, 2);
}
