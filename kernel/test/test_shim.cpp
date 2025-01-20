#include <gtest/gtest.h>

#include "kernel.hpp"

class TestStream final : public km::IOutStream {
public:
    void write(stdx::StringView message) override {
        for (char c : message) {
            if (c == '\0') continue;
            std::cout << c;
        }
    }
};

void km::LockDebugLog() { }
void km::UnlockDebugLog() { }

km::IOutStream *km::GetDebugStream() {
    static TestStream sTestStream;
    return &sTestStream;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn" // GTEST_FAIL_AT is a macro that doesn't return

void KmBugCheck(stdx::StringView message, stdx::StringView file, unsigned line) {
    GTEST_FAIL() << "Bugcheck: " << std::string_view(message) << " at " << std::string_view(file) << ":" << line;
}

#pragma clang diagnostic pop
