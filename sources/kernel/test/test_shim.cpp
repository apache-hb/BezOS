#include <cstdarg>
#include <gtest/gtest.h>

#include "absl/base/internal/raw_logging.h"
#include "absl/base/internal/throw_delegate.h"

#include "isr/isr.hpp"
#include "kernel.hpp"

#include "absl/hash/internal/city.cc"
#include "absl/hash/internal/hash.cc"
#include "absl/hash/internal/low_level_hash.cc"
#include "absl/container/internal/raw_hash_set.cc"
#include "processor.hpp"

void KmIdle() {
    while (1) { }
}

void KmHalt() {
    while (1) { }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn" // GTEST_FAIL_AT is a macro that doesn't return
#pragma clang diagnostic ignored "-Wfunction-effects" // bugCheck isnt actually nonblocking

void km::BugCheck(stdx::StringView message, std::source_location where) noexcept [[clang::nonblocking]] {
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

km::CpuCoreId km::GetCurrentCoreId() noexcept [[clang::reentrant]] {
    return km::CpuCoreId(0);
}
