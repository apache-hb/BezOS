#include <gtest/gtest.h>

#include "isr/isr.hpp"

TEST(IsrTableTest, Construct) {
    [[maybe_unused]]
    km::LocalIsrTable table;
}

static km::IsrContext TestIsrHandler(km::IsrContext *context) {
    return *context;
}

TEST(IsrTableTest, Allocate) {
    km::LocalIsrTable table;

    auto isr = table.allocate(TestIsrHandler);
    EXPECT_NE(isr, nullptr);

    isr = table.allocate(TestIsrHandler);
    EXPECT_NE(isr, nullptr);

    isr = table.allocate(TestIsrHandler);
    EXPECT_NE(isr, nullptr);
}
