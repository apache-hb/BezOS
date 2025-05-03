#include <gtest/gtest.h>

#include "pvtest/machine.hpp"

class PvConstructTest : public testing::Test {
public:
};

TEST_F(PvConstructTest, Construct) {
    pv::Machine::init();
    pv::Machine *machine = new pv::Machine(4);
    EXPECT_NE(machine, nullptr);
    delete machine;
}
