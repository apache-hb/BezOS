#pragma once

#include "boot.hpp"
#include "std/string.hpp"
#include "util/format.hpp"

#define KTEST_SECTION ".testing"

namespace km::testing {
    namespace detail {
        inline constexpr size_t kAssertMessageSize = 1024;
        using AssertMessage = stdx::StaticString<kAssertMessageSize>;
        void addError(AssertMessage message, std::source_location location = std::source_location::current());
    }

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

    void initKernelTest(boot::LaunchInfo launch);
    int runAllTests();
}

#define KTEST_F(FixtureClass, TestName) \
    class Test_##FixtureClass##_##TestName : public FixtureClass { \
        [[gnu::section(KTEST_SECTION)]] \
        constinit static volatile const km::testing::TestFactory kFactory; \
    public: \
        void TestBody() override; \
    }; \
    [[gnu::used, gnu::section(KTEST_SECTION)]] \
    constinit const volatile km::testing::TestFactory Test_##FixtureClass##_##TestName::kFactory = []() -> km::testing::Test* { \
        return new Test_##FixtureClass##_##TestName(); \
    }; \
    void Test_##FixtureClass##_##TestName::TestBody()

#define KASSERT_EQ(expected, actual) \
    do { \
        auto e = (expected); \
        auto a = (actual); \
        if (e != a) { \
            km::testing::detail::addError(km::concat<km::testing::detail::kAssertMessageSize>("Assertion failed: " #expected " != " #actual " (expected ", e, ", got ", a, ")")); \
        } \
    } while (0)
