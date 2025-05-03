#include <gtest/gtest.h>

#include "pvtest/machine.hpp"

class PvConstructTest : public testing::Test {
public:
};

TEST_F(PvConstructTest, Construct) {
    pv::Machine::init();
    pv::Machine machine(4);
}
