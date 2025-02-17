#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsAddressSpace Address Space
/// @{

enum {
    eOsMemoryNoAccess = 0,
    eOsMemoryRead     = (1 << 0),
    eOsMemoryWrite    = (1 << 1),
    eOsMemoryExecute  = (1 << 2),
};

typedef uint32_t OsMemoryAccess;

struct OsAddressSpaceCreateInfo {
    const char *NameFront;
    const char *NameBack;

    OsSize Size;
    OsMemoryAccess Access;
};

extern OsStatus OsAddressSpaceCreate(struct OsAddressSpaceCreateInfo CreateInfo, OsAddressSpaceHandle *OutHandle);

/// @} // group OsAddressSpaceHandle

#ifdef __cplusplus
}
#endif
