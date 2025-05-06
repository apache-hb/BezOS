#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    eOsPropertyNone = 0,
};

typedef uint32_t OsPropertyKey;
typedef uint64_t OsPropertyValue;

extern OsStatus OsPropertyGet(OsPropertyKey Key, OsPropertyValue *OutValue);

#ifdef __cplusplus
}
#endif
