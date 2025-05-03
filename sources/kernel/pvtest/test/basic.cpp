#include <gtest/gtest.h>

#include "pvtest/machine.hpp"

class PvConstructTest : public testing::Test {
public:
};

TEST_F(PvConstructTest, Construct) {
    if (pid_t child = fork()) {
        pv::Machine::init();
        pv::Machine machine(4);
    } else {

        sigset_t set;
        sigprocmask(SIG_BLOCK, nullptr, &set);
        sigaddset(&set, SIGUSR1);
        sigaddset(&set, SIGUSR2);
        sigprocmask(SIG_SETMASK, &set, nullptr);

        int status;
        waitpid(child, &status, 0);
    }
}
