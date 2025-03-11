#include "private.hpp"

#include <bezos/facility/debug.h>

#include <posix/string.h>
#include <posix/errno.h>

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
