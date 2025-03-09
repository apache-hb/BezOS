#include <bezos/facility/handle.h>

#include <bezos/private.h>

OsStatus OsHandleWait(OsHandle Handle, OsInstant Timeout) {
    struct OsCallResult result = OsSystemCall(eOsCallHandleWait, (uintptr_t)Handle, Timeout, 0, 0);
    return result.Status;
}
