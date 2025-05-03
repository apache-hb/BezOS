#include <gtest/gtest.h>

#include "pvtest/machine.hpp"

class PvConstructTest : public testing::Test {
public:
};

TEST_F(PvConstructTest, Construct) {
    pv::Machine::init();
    auto shared = pv::Machine::getSharedMemory();
    EXPECT_TRUE(shared.isValid()) << "Shared memory must be valid " << std::string_view(km::format(shared));

    pv::Machine *machine = new pv::Machine(4);
    EXPECT_NE(machine, nullptr);
    EXPECT_TRUE(shared.contains((void*)machine)) << "Machine " << (void*)machine << " must be allocated in shared memory " << std::string_view(km::format(shared));
    delete machine;

    pv::Machine::finalize();
}
