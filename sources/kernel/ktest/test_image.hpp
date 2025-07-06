#pragma once

#include "boot.hpp"
#include "logger/logger.hpp"

constinit inline km::Logger TestLog { "TEST" };

#define KTEST_SECTION __attribute__((section(".rodata.testing")))

namespace km::testing {
    class Test {
    public:
        virtual ~Test() = default;

        void Run();

    protected:
        virtual void SetUp() {
            // Default setup, can be overridden by derived classes
        }

        virtual void TearDown() {
            // Default teardown, can be overridden by derived classes
        }

    private:
        virtual void TestBody() = 0;
    };

    using TestFactory = Test* (*)();

    void InitKernelTest(boot::LaunchInfo launch);
    int RunAllTests();
}

#define KTEST_F(FixtureClass, TestName) \
    class Test_##FixtureClass##_##TestName : public FixtureClass { \
        static const km::testing::TestFactory kFactory; \
    public: \
        void TestBody() override; \
    }; \
    constexpr km::testing::TestFactory Test_##FixtureClass##_##TestName::kFactory = []() -> km::testing::Test* { \
        return new Test_##FixtureClass##_##TestName(); \
    }; \
    void Test_##FixtureClass##_##TestName::TestBody()
