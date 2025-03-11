#include <bezos/facility/debug.h>

#include <bezos/private.h>

OsStatus OsDebugMessage(struct OsDebugMessageInfo Message) {
    struct OsCallResult result = OsSystemCall(eOsCallDebugMessage, (uint64_t)&Message, 0, 0, 0);
    return result.Status;
}
