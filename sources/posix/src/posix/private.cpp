#include "private.hpp"

#include <bezos/facility/debug.h>

#include <posix/string.h>
#include <posix/errno.h>
#include <posix/stdio.h>
#include <posix/stdlib.h>

static void WriteMessage(OsDebugMessageFlags flags, const char *front, const char *back) {
    OsDebugMessageInfo messageInfo {
        .Front = front,
        .Back = back,
        .Info = flags,
    };

    OsDebugMessage(messageInfo);
}

template<size_t N>
static void WriteMessage(OsDebugMessageFlags flags, const char (&message)[N]) {
    WriteMessage(flags, message, message + N - 1);
}

void Unimplemented(int error, std::source_location location) {
    DebugLog(eOsLogError, "POSIX unimplemented function invoked: %s in %s:%d", location.function_name(), location.file_name(), location.line());
    errno = error;
}

void Unimplemented(int error, const char *message, std::source_location location) {
    DebugLog(eOsLogError, "POSIX unimplemented function invoked: %s in %s:%d", message, location.file_name(), location.line());
    errno = error;
}

void DebugLog(OsDebugMessageFlags Flags, const char *format, ...) {
    char buffer[1024];
    int status;
    va_list args;
    va_start(args, format);
    status = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (status < 0) {
        WriteMessage(eOsLogError, "POSIX: Failed to format message");
        return;
    }

    const char *front = buffer;
    const char *back = buffer + strnlen(buffer, sizeof(buffer));

    WriteMessage(Flags, front, back);
}

void AssertOsSuccess(OsStatus status, const char *expr, std::source_location location) {
    if (status != OsStatusSuccess) {
        DebugLog(eOsLogCritical, "POSIX: Assertion failed: %s = %d in %s:%d", expr, status, location.file_name(), location.line());
        abort();
    }
}
