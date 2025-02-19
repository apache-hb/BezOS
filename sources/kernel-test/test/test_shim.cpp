#include <gtest/gtest.h>

#include <cstdarg>
#include <source_location>

#include "absl/base/internal/raw_logging.h"
#include "absl/base/internal/throw_delegate.h"

#include "isr.hpp"

#include "log.hpp"
#include "panic.hpp"

#include "absl/hash/internal/city.cc"
#include "absl/hash/internal/hash.cc"
#include "absl/hash/internal/low_level_hash.cc"
#include "absl/container/internal/raw_hash_set.cc"

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

void km::BugCheck(stdx::StringView message, std::source_location where) {
    GTEST_FAIL() << "Bugcheck: " << std::string_view(message) << " at " << where.file_name() << ":" << where.line();
}

#pragma clang diagnostic pop

void DefaultInternalLog(absl::LogSeverity, const char *, int, std::string_view message) {
    std::cout << "Internal log: " << message << std::endl;
}

constinit absl::base_internal::AtomicHook<absl::raw_log_internal::InternalLogFunction>
    absl::raw_log_internal::internal_log_function(DefaultInternalLog);

void km::DisableInterrupts() { }
void km::EnableInterrupts() { }

void absl::base_internal::ThrowStdOutOfRange(const char *message) {
    throw std::out_of_range(message);
}

void absl::raw_log_internal::RawLog(absl::LogSeverity, const char *file, int line, const char *, ...) {
    GTEST_FAIL() << "RawLog: " << file << ":" << line;
}
