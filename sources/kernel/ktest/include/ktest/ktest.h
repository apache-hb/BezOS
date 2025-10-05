#pragma once

#include "std/string.hpp" // IWYU pragma: keep
#include "util/format.hpp" // IWYU pragma: keep

#include <source_location>

#define KM_TEST_SECTION ".testing"

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
}

#define KTEST_DECLARE_TEST(SuiteName, TestName, ParentClass) \
    class Test_##SuiteName##_##TestName : public ParentClass { \
        [[gnu::section(KM_TEST_SECTION)]] \
        constinit static volatile const km::testing::TestFactory kFactory; \
    public: \
        void TestBody() override; \
    }; \
    [[gnu::used, gnu::section(KM_TEST_SECTION)]] \
    constinit const volatile km::testing::TestFactory Test_##SuiteName##_##TestName::kFactory = []() -> km::testing::Test* { \
        return new Test_##SuiteName##_##TestName(); \
    }; \
    void Test_##SuiteName##_##TestName::TestBody()

#define KTEST(SuiteName, TestName) KTEST_DECLARE_TEST(SuiteName, TestName, km::testing::Test)

#define KTEST_F(FixtureClass, TestName) KTEST_DECLARE_TEST(FixtureClass, TestName, FixtureClass)

#define KASSERT_EQ(expected, actual) \
    do { \
        auto e = (expected); \
        auto a = (actual); \
        if (e != a) { \
            km::testing::detail::addError(km::concat<km::testing::detail::kAssertMessageSize>("Assertion failed: " #expected " != " #actual " (expected ", e, ", got ", a, ")")); \
            return; \
        } \
    } while (0)
