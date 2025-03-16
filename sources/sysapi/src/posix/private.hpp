#pragma once

#include <bezos/status.h>
#include <bezos/facility/debug.h>

#include <posix/errno.h>

#include <source_location>

[[gnu::visibility("hidden")]]
void Unimplemented(int error = ENOSYS, std::source_location location = std::source_location::current());

[[gnu::visibility("hidden")]]
void Unimplemented(int error, const char *message, std::source_location location = std::source_location::current());

[[gnu::visibility("hidden")]]
void DebugLog(OsDebugMessageFlags Flags, const char *format, ...);

[[gnu::visibility("hidden")]]
void AssertOsSuccess(OsStatus status, const char *expr, std::source_location location = std::source_location::current());

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, #expr)
