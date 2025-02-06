#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t OsStatus;

enum OsStatusId {
    OsStatusSuccess = 0,

    OsStatusOutOfMemory = 1,

    OsStatusNotFound = 2,

    OsStatusInvalidInput = 3,

    OsStatusNotSupported = 4,
};

inline bool OsStatusOk(OsStatus status) {
    return status == OsStatusSuccess;
}

inline bool OsStatusError(OsStatus status) {
    return !OsStatusOk(status);
}

#ifdef __cplusplus
}
#endif
