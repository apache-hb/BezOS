#include <gtest/gtest.h>

#include "common/compiler/compiler.hpp"

// stub tests to ensure the compiler platform is implemented
TEST(CompilerTest, Version) {
    auto version = sm::Compiler::GetVersion();
    EXPECT_NE(version.major(), 0);
}

TEST(CompilerTest, Name) {
    auto name = sm::Compiler::GetName();
    EXPECT_FALSE(name.empty());
}
