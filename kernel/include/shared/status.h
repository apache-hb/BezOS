#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum OsStatus {
    OsStatusSuccess = 0,

    OsStatusOutOfMemory = 1,

    OsStatusNotFound = 2,

    OsStatusInvalidInput = 3,

    OsStatusNotSupported = 4,
};

#ifdef __cplusplus
}
#endif
