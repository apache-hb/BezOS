#include "pvtest/msr.hpp"
#include <gtest/gtest.h>

#include "pvtest/machine.hpp"

class TestMachine : public pv::Machine {
    size_t mReadCount = 0;
    size_t mWriteCount = 0;

    void wrmsr(uint32_t msr, uint64_t value) override {
        mWriteCount += 1;

        EXPECT_EQ(value, msr * 2);
    }

    uint64_t rdmsr(uint32_t msr) override {
        mReadCount += 1;

        EXPECT_EQ(msr, 0x1234);
        return 0x1234 * 2;
    }

public:
    using Machine::Machine;
    size_t getReadCount() const { return mReadCount; }
    size_t getWriteCount() const { return mWriteCount; }
};

class PvMsrTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        pv::Machine::init();
    }

    static void TearDownTestSuite() {
        pv::Machine::finalize();
    }

    void SetUp() override {
        machine = new TestMachine(1, sm::gigabytes(1).bytes());
        ASSERT_NE(machine, nullptr);

        machine->initSingleThread();
    }

    void TearDown() override {
        delete machine;
    }

    TestMachine *machine = nullptr;
};

TEST_F(PvMsrTest, BasicRdmsr) {
    ASSERT_EQ(machine->getReadCount(), 0);
    ASSERT_EQ(machine->getWriteCount(), 0);

    arch::IntrinX86_64::wrmsr(1000, 2000);

    ASSERT_EQ(machine->getReadCount(), 0);
    ASSERT_EQ(machine->getWriteCount(), 1);

    uint64_t value = arch::IntrinX86_64::rdmsr(0x1234);
    ASSERT_EQ(value, 0x1234 * 2);

    ASSERT_EQ(machine->getReadCount(), 1);
    ASSERT_EQ(machine->getWriteCount(), 1);
}
