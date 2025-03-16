#include "private.hpp"

#include <bezos/facility/debug.h>

#include <posix/string.h>
#include <posix/errno.h>
#include <posix/stdio.h>
#include <posix/stdlib.h>

void Unimplemented(int error, std::source_location location) {
    char buffer[256]{};
    strcat(buffer, "POSIX unimplemented function invoked: ");
    strcat(buffer, location.function_name());
    OsDebugMessageInfo messageInfo {
        .Front = buffer,
        .Back = buffer + strlen(buffer),
        .Info = eOsLogError,
    };

    errno = error;
    OsDebugMessage(messageInfo);
}

void Unimplemented(int error, const char *message, std::source_location location) {
    char buffer[256]{};
    strcat(buffer, "POSIX internal assertion: ");
    strncat(buffer, message, 128);
    strcat(buffer, " in ");
    strcat(buffer, location.function_name());
    strcat(buffer, ": ");
    strcat(buffer, message);
    OsDebugMessageInfo messageInfo {
        .Front = buffer,
        .Back = buffer + strlen(buffer),
        .Info = eOsLogError,
    };

    errno = error;
    OsDebugMessage(messageInfo);
}

void DebugLog(OsDebugMessageFlags Flags, const char *format, ...) {
    char buffer[1024]{};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    OsDebugMessageInfo messageInfo {
        .Front = buffer,
        .Back = buffer + strlen(buffer),
        .Info = Flags,
    };

    OsDebugMessage(messageInfo);
}

void AssertOsSuccess(OsStatus status, const char *expr, std::source_location location) {
    if (status != OsStatusSuccess) {
        DebugLog(eOsLogCritical, "POSIX: Assertion failed: %s in %s:%d", expr, location.file_name(), location.line());
        abort();
    }
}
