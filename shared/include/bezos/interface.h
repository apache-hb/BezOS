#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsInterface Interface
/// @{

struct OsInterface {
    OsStatus (*Identify)(void *Instance, struct OsGuid *OutGuid, char *NameFront, char *NameBack);
    OsStatus (*QueryInterface)(void *Instance, const struct OsGuid *InterfaceId, void **OutInterface);
};

#ifdef __cplusplus
struct IOsInterface {
    virtual ~IOsInterface() = 0;

    virtual OsStatus Identify(struct OsGuid *OutGuid, char *NameFront, char *NameBack) = 0;
    virtual OsStatus QueryInterface(const struct OsGuid *InterfaceId, void **OutInterface) = 0;
};
#endif

extern OsStatus OsCreateInterface(const struct OsGuid *Guid, struct OsInterface **OutInterface);

/// @}

#ifdef __cplusplus
}
#endif
