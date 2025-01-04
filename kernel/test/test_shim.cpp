#include <gtest/gtest.h>

#include "kernel.hpp"

void KmDebugWrite(stdx::StringView) { }
void KmBeginWrite() { }
void KmEndWrite() { }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn" // GTEST_FAIL_AT is a macro that doesn't return

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    GTEST_FAIL_AT(std::string(file.begin(), file.end()).c_str(), line)
        << "Bug check triggered: " << std::string_view(message)
        << " at " << std::string_view(file) << ":" << line;
}

#pragma clang diagnostic pop
